#pragma once
#include <ArduinoJson.h>
#include "control.h"
namespace settings {
using namespace avicole;
inline int choice(const String& s,const char* const* names,int n){for(int i=0;i<n;i++)if(s==names[i])return i;return -1;}
inline bool uintIn(JsonVariantConst v,uint32_t lo,uint32_t hi){return v.is<uint32_t>()&&v.as<uint32_t>()>=lo&&v.as<uint32_t>()<=hi;}
inline bool rate(int b){return b==1200||b==2400||b==4800||b==9600||b==19200||b==38400||b==57600||b==115200;}
inline String parse(JsonDocument& d,Config& out){
 if(d["version"]!=1||!d["hardwareConfirmed"].is<bool>()||!d["busCommissioned"].is<bool>())return "Configuration matérielle invalide";
 if(!d["species"].is<const char*>()||String(d["species"].as<const char*>()).length()<1||String(d["species"].as<const char*>()).length()>80||!uintIn(d["ageDays"],0,9999))return "Espèce ou âge invalide";
 if(!uintIn(d["staleMs"],1000,600000)||!uintIn(d["pollMs"],1000,60000)||d["staleMs"].as<int>()<=d["pollMs"].as<int>()||!uintIn(d["responseTimeoutMs"],200,5000)||!uintIn(d["interTransactionMs"],200,5000))return "Délais invalides (minimum 200 ms)";
 for(const char* key:{"ts","nh"}){auto s=d["sensors"][key];if(!uintIn(s["address"],1,247)||!s["baud"].is<int>()||!rate(s["baud"]))return "Paramètres Modbus invalides";}
 auto ts=d["sensors"]["ts"],nh=d["sensors"]["nh"];
 if(ts["address"].as<int>()==nh["address"].as<int>()||ts["baud"].as<int>()!=nh["baud"].as<int>())return "Adresses distinctes et vitesse commune requises";
 int range=nh["rangePpm"]|0;if((range!=50&&range!=100&&range!=500&&range!=1000)||!nh["combined"].is<bool>()||!nh["modelConfirmed"].is<bool>()||!nh["model"].is<const char*>()||String(nh["model"].as<const char*>()).length()<1||String(nh["model"].as<const char*>()).length()>100)return "Modèle NH3 invalide";
 if(!d["profiles"].is<JsonArray>()||d["profiles"].size()<1||d["profiles"].size()>12)return "1 à 12 profils requis";
 JsonObject selected;int index=0;
 for(JsonObject p:d["profiles"].as<JsonArray>()){
  if(!p["species"].is<const char*>()||String(p["species"].as<const char*>()).length()<1||String(p["species"].as<const char*>()).length()>80||!uintIn(p["minAgeDays"],0,9999)||!uintIn(p["maxAgeDays"],p["minAgeDays"],9999)||!p["validated"].is<bool>())return "Profil invalide";
  auto rules=p["rules"];for(const char* k:{"hot","humid","co2","nh3","cold","dry"}){if(!rules[k]["value"].is<float>()||!rules[k]["hysteresis"].is<float>()||!std::isfinite(rules[k]["value"].as<float>())||!std::isfinite(rules[k]["hysteresis"].as<float>())||rules[k]["hysteresis"].as<float>()<=0)return "Seuil/hystérésis invalide";}
  if(rules["cold"]["value"].as<float>()+rules["cold"]["hysteresis"].as<float>()>=rules["hot"]["value"].as<float>()-rules["hot"]["hysteresis"].as<float>()||rules["dry"]["value"].as<float>()+rules["dry"]["hysteresis"].as<float>()>=rules["humid"]["value"].as<float>()-rules["humid"]["hysteresis"].as<float>())return "Bandes de régulation en conflit";
  if(rules["cold"]["value"].as<float>()< -40||rules["hot"]["value"].as<float>()>120||rules["dry"]["value"].as<float>()<0||rules["humid"]["value"].as<float>()>100||rules["co2"]["value"].as<float>()<0||rules["co2"]["value"].as<float>()>65535||rules["nh3"]["value"].as<float>()<0||rules["nh3"]["value"].as<float>()>range)return "Consigne hors plage";
  if(!uintIn(rules["minVentPeriodMs"],1000,86400000)||!uintIn(rules["minVentOnMs"],0,rules["minVentPeriodMs"]))return "Cycle de ventilation invalide";
  for(int j=0;j<index;j++){auto q=d["profiles"][j];if(p["species"].as<String>()==q["species"].as<String>()&&p["minAgeDays"].as<int>()<=q["maxAgeDays"].as<int>()&&q["minAgeDays"].as<int>()<=p["maxAgeDays"].as<int>())return "Profils se chevauchant";}
  if(p["species"].as<String>()==d["species"].as<String>()&&d["ageDays"].as<int>()>=p["minAgeDays"].as<int>()&&d["ageDays"].as<int>()<=p["maxAgeDays"].as<int>())selected=p;++index;
 }
 if(selected.isNull())return "Aucun profil pour cet âge/espèce";
 Config c;c.staleMs=d["staleMs"];c.validated=selected["validated"].as<bool>()&&d["hardwareConfirmed"].as<bool>()&&d["busCommissioned"].as<bool>()&&nh["modelConfirmed"].as<bool>();auto r=selected["rules"];
 Threshold* thresholds[]={&c.hot,&c.humid,&c.co2,&c.nh3,&c.cold,&c.dry};const char* keys[]={"hot","humid","co2","nh3","cold","dry"};for(int i=0;i<6;i++)*thresholds[i]={r[keys[i]]["value"],r[keys[i]]["hysteresis"]};c.minVentPeriodMs=r["minVentPeriodMs"];c.minVentOnMs=r["minVentOnMs"];
 const char* roles[]={"vent_main","vent_extra","heating","humidifier","cooling","lighting","alarm","unused"};const char* policies[]={"off","on","hold"};bool assigned[8]={};
 if(!d["relays"].is<JsonArray>()||d["relays"].size()!=6)return "Six relais requis";
 for(int i=0;i<6;i++){auto s=d["relays"][i];int role=choice(s["role"].as<String>(),roles,8),boot=choice(s["startup"].as<String>(),policies,3),fault=choice(s["sensorFault"].as<String>(),policies,3),obs=choice(s["observation"].as<String>(),policies,3);if(role<0||boot<0||fault<0||obs<0||!s["confirmed"].is<bool>()||!s["policyConfirmed"].is<bool>()||!uintIn(s["minOnMs"],0,3600000)||!uintIn(s["minOffMs"],0,3600000)||s["network"].as<String>()!="continue_local"||!s["name"].is<const char*>()||String(s["name"].as<const char*>()).length()<1||String(s["name"].as<const char*>()).length()>80)return "Relais/politique invalide";if(role!=7&&assigned[role])return "Une seule sortie par rôle";assigned[role]=true;c.relays[i]={Role(role),s["confirmed"],s["policyConfirmed"],s["minOnMs"],s["minOffMs"],Policy(boot),Policy(fault),Policy(obs)};}
 out=c;return "";
}
}