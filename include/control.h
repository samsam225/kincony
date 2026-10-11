#pragma once
#include "modbus.h"
#include <cmath>
#include <array>
namespace avicole {
enum class Mode { Observation, Manual, Automatic };
enum class Role { VentMain, VentExtra, Heating, Humidifier, Cooling, Lighting, Alarm, Unused };
enum class Policy { Off, On, Hold };
enum class Reason { Unconfirmed, Startup, Observation, SensorFault, Manual, Automatic, Conflict, Timer, Pulse };
inline const char* reasonName(Reason r){const char* v[]={"Non confirmé","Démarrage","Observation","Défaut capteur","Manuel","Automatique","Conflit prioritaire","Temporisation","Impulsion de test"};return v[int(r)];}
struct Measurement {
 float value=NAN;uint64_t last=0;bool seen=false;Error error=Error::Unconfirmed;
 void accept(float v,uint64_t now){value=v;last=now;seen=true;error=Error::Ok;}
 bool valid(uint64_t now,uint32_t stale)const{return seen&&std::isfinite(value)&&error==Error::Ok&&now-last<stale;}
};
struct Warmup {
 uint64_t start=0;bool started=false,explicitlyReady=false;
 void restart(uint64_t now){start=now;started=true;explicitlyReady=false;}
 bool ready(uint64_t now,int range)const{return explicitlyReady||(started&&now-start>=nhWarmupMs(range));}
};
struct Threshold {float value=0,hysteresis=1;};
struct RelayConfig {
 Role role=Role::Unused;bool confirmed=false,policyConfirmed=false;
 uint32_t minOnMs=30000,minOffMs=30000;
 Policy startup=Policy::Off,sensorFault=Policy::Off,observation=Policy::Off;
};
struct Config {
 bool validated=false;uint32_t staleMs=15000;
 Threshold hot{30,1},humid{80,5},co2{1500,100},nh3{15,2},cold{20,1},dry{40,5};
 uint32_t minVentPeriodMs=300000,minVentOnMs=60000;
 std::array<RelayConfig,6> relays{};
};
struct RelayState {bool command=false;uint64_t changed=0;Reason reason=Reason::Unconfirmed;bool writeAcknowledged=false;};
class Controller {
 bool hot=false,humid=false,co2=false,nh3=false,cold=false,dry=false;
 bool startup=true;std::array<bool,6> manual{};uint64_t pulseEnd=0;int pulseIndex=-1;
 static bool high(bool latched,float v,Threshold t){return latched?v>t.value-t.hysteresis:v>=t.value;}
 static bool low(bool latched,float v,Threshold t){return latched?v<t.value+t.hysteresis:v<=t.value;}
 static bool policy(Policy p,bool current){return p==Policy::On||(p==Policy::Hold&&current);}
 public:
 Config config;Mode mode=Mode::Observation;std::array<Measurement,4> measures;std::array<RelayState,6> relays;
 bool network=true; // Intentionnellement sans effet sur les demandes locales.
 void restart(uint64_t now){mode=Mode::Observation;startup=true;manual.fill(false);pulseIndex=-1;hot=humid=co2=nh3=cold=dry=false;for(auto& m:measures)m=Measurement{};for(auto& r:relays){r=RelayState{};r.changed=now;}}
 void configure(Config c,uint64_t now){config=c;restart(now);}
 bool setMode(Mode m){if(m==Mode::Automatic&&!config.validated)return false;mode=m;startup=false;manual.fill(false);pulseIndex=-1;return true;}
 bool manualCommand(int i,bool on){if(i<0||i>=6||mode!=Mode::Manual||!config.relays[i].confirmed||!config.relays[i].policyConfirmed)return false;manual[i]=on;return true;}
 bool pulse(int i,uint32_t duration,uint64_t now){if(i<0||i>=6||duration<100||duration>2000||mode!=Mode::Observation||!config.relays[i].confirmed||!config.relays[i].policyConfirmed||config.relays[i].observation!=Policy::Off||pulseIndex>=0)return false;for(auto& r:relays)if(r.command)return false;pulseIndex=i;pulseEnd=now+duration;startup=false;return true;}
 bool fault(uint64_t now)const{for(auto& m:measures)if(!m.valid(now,config.staleMs))return true;return false;}
 bool gasDemand()const{return co2||nh3;}
 bool boostDemand()const{return hot||humid||co2||nh3;}
 void tick(uint64_t now){
  const bool bad=fault(now);if(!bad){hot=high(hot,measures[0].value,config.hot);humid=high(humid,measures[1].value,config.humid);co2=high(co2,measures[2].value,config.co2);nh3=high(nh3,measures[3].value,config.nh3);cold=low(cold,measures[0].value,config.cold);dry=low(dry,measures[1].value,config.dry);}else{hot=humid=co2=nh3=cold=dry=false;}
  bool expired=pulseIndex>=0&&now>=pulseEnd;int stopped=expired?pulseIndex:-1;if(expired)pulseIndex=-1;
  bool boost=hot||humid||co2||nh3;
  bool minimum=config.minVentPeriodMs>0&&now%config.minVentPeriodMs<config.minVentOnMs;
  for(int i=0;i<6;i++){
   auto& c=config.relays[i];auto& r=relays[i];bool desired=false,force=false;Reason why=Reason::Automatic;
   if(!c.confirmed||!c.policyConfirmed||c.role==Role::Unused){why=Reason::Unconfirmed;force=true;}
   else if(startup){desired=policy(c.startup,r.command);why=Reason::Startup;force=true;}
   else if(pulseIndex==i){desired=true;why=Reason::Pulse;force=true;}
   else if(stopped==i){desired=false;why=Reason::Pulse;force=true;}
   else if(mode==Mode::Observation){desired=policy(c.observation,r.command);why=Reason::Observation;force=true;}
   else if(bad){desired=policy(c.sensorFault,r.command);why=Reason::SensorFault;force=true;}
   else if(mode==Mode::Manual){desired=manual[i];why=Reason::Manual;}
   else if(!config.validated){why=Reason::Observation;force=true;}
   else switch(c.role){case Role::VentMain:desired=minimum||boost;break;case Role::VentExtra:desired=boost;break;case Role::Heating:desired=cold&&!boost;break;case Role::Humidifier:desired=dry&&!boost&&!cold;break;case Role::Cooling:desired=hot&&!cold;break;case Role::Lighting:desired=false;break;case Role::Alarm:desired=boost;break;default:break;}
   // Gaz/ventilation prioritaire ; chauffage et humidification ne restent pas
   // enclenchés durant une temporisation de ventilation ou un mode manuel.
   bool vent=false,heat=false,cool=false;
   for(int j=0;j<6;j++){auto role=config.relays[j].role;if(!config.relays[j].confirmed)continue;if(role==Role::VentMain||role==Role::VentExtra)vent|=relays[j].command;if(role==Role::Heating)heat|=relays[j].command;if(role==Role::Cooling)cool|=relays[j].command;}
   if((desired||r.command)&&((c.role==Role::Heating&&(boost||vent||cool))||(c.role==Role::Humidifier&&(boost||vent||heat||cool))||(c.role==Role::Cooling&&heat))){desired=false;force=true;why=Reason::Conflict;}
   if(desired!=r.command){auto duration=r.command?c.minOnMs:c.minOffMs;if(force||now-r.changed>=duration){r.command=desired;r.changed=now;r.writeAcknowledged=false;}else why=Reason::Timer;}
   r.reason=why;
  }
  // Seconde passe indépendante de l’ordre d’affectation : un ventilateur
  // demandé ne peut coexister avec chauffage/humidification déjà commandés.
  bool vent=false,heat=false,cool=false;for(int i=0;i<6;i++){if(config.relays[i].role==Role::VentMain||config.relays[i].role==Role::VentExtra)vent|=relays[i].command;if(config.relays[i].role==Role::Heating)heat|=relays[i].command;if(config.relays[i].role==Role::Cooling)cool|=relays[i].command;}
  for(int i=0;i<6;i++){auto role=config.relays[i].role;if(relays[i].command&&((role==Role::Heating&&(vent||cool))||(role==Role::Humidifier&&(vent||heat||cool)))){relays[i].command=false;relays[i].changed=now;relays[i].reason=Reason::Conflict;relays[i].writeAcknowledged=false;}}
 }
};
}
