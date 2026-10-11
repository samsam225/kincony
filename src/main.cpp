#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "hardware.h"
#include "settings.h"
#include "generated.h"
using namespace avicole;
WebServer server(80);Preferences prefs;HardwareSerial rs485(1);Controller ctl;Warmup warm;JsonDocument config;
SemaphoreHandle_t mutex;struct Guard{Guard(){xSemaphoreTake(mutex,portMAX_DELAY);}~Guard(){xSemaphoreGive(mutex);}};
uint64_t nowMs=0;uint32_t lastClock=0;String userName,password,apPassword,csrf;bool ioStarted=false,ioOk=false;uint8_t lastOutput=0xff;
struct Event{uint64_t time=0;String type,detail;};Event events[128];unsigned eventHead=0,eventUsed=0;
struct Sample{uint64_t time=0;float v[4];uint8_t valid=0;};Sample samples[360];unsigned sampleHead=0,sampleUsed=0;uint64_t lastSample=0;String lastAlarm;
void logEvent(const String& type,const String& detail){events[eventHead]={nowMs,type,detail};eventHead=(eventHead+1)%128;if(eventUsed<128)eventUsed++;}
struct Job{bool queued=false,pending=false,complete=false;int owner=0;uint32_t baud=9600;std::array<uint8_t,8> req{};uint8_t response[32]={};size_t received=0;uint32_t txEndUs=0,lastRxUs=0;uint64_t sentMs=0;Error error=Error::Ok;uint8_t exception=0;};Job job;
uint32_t busBaud=9600,lastActivityUs=0;uint64_t nextBus=0,nextPoll=0;int pollSensor=0;
struct Diagnostic{uint32_t success=0,timeout=0,crc=0,exception=0;String lastTx,lastRx,status="Repos";bool commissionDone=false,commissionVerified=false,nhRestartRequired=false;String commissionSensor,commissionAction;};Diagnostic diag;
String hex(const uint8_t* p,size_t n){String s;for(size_t i=0;i<n;i++){if(i)s+=' ';if(p[i]<16)s+='0';s+=String(p[i],HEX);}return s;}
void markSensorError(int owner,Error e){if(owner==0){for(int i=0;i<3;i++)ctl.measures[i].error=e;}else if(owner==1)ctl.measures[3].error=e;}
void queueRead(int owner,uint8_t addr,uint32_t baud,uint16_t reg,uint16_t count,uint8_t fn=3){job=Job{};job.owner=owner;job.baud=baud;job.req=request(addr,fn,reg,count);job.queued=true;}
void completed(Error e){
 job.error=e;job.pending=false;job.queued=false;job.complete=true;nextBus=nowMs+config["interTransactionMs"].as<uint32_t>();lastActivityUs=micros();diag.status=errorName(e);diag.lastRx=hex(job.response,job.received);
 if(e==Error::Ok)diag.success++;else if(e==Error::Timeout)diag.timeout++;else if(e==Error::Crc)diag.crc++;else if(e==Error::Exception)diag.exception++;
 if(job.owner<2){
  if(e!=Error::Ok){markSensorError(job.owner,e);return;}
  if(job.owner==0){auto d=decodeTs(job.response+3);float v[]={d.temperature,d.humidity,d.co2};for(int i=0;i<3;i++)ctl.measures[i].accept(v[i],nowMs);if(d.temperature< -40||d.temperature>120)ctl.measures[0].error=Error::Range;if(d.humidity<0||d.humidity>100)ctl.measures[1].error=Error::Range;if(d.co2<400||d.co2>5000)ctl.measures[2].error=Error::Range;}
  else{auto nh=config["sensors"]["nh"];auto d=decodeNh(job.response+3,nh["rangePpm"],nh["combined"]);ctl.measures[3].accept(d.nh3,nowMs);if(!nh["modelConfirmed"].as<bool>())ctl.measures[3].error=Error::Unconfirmed;else if(!warm.ready(nowMs,nh["rangePpm"]))ctl.measures[3].error=Error::Warming;else if(d.nh3>nh["rangePpm"].as<float>())ctl.measures[3].error=Error::Range;}
 }else{
  diag.commissionDone=true;diag.commissionVerified=false;
  if(e==Error::Ok&&diag.commissionAction=="verify"){
   bool ts=diag.commissionSensor=="ts";diag.commissionVerified=word(job.response+3)==(ts?1:2)&&word(job.response+5)==(ts?3:2);if(!diag.commissionVerified)diag.status="Relecture différente de la cible";else if(!ts)diag.nhRestartRequired=false;
  }
  logEvent("mise en service",diag.commissionSensor+" / "+diag.commissionAction+" : "+diag.status+(diag.commissionVerified?" ; cible vérifiée":""));
 }
}
void busTick(){
 const uint32_t us=micros();
 if(job.pending){
  while(rs485.available()){
   uint32_t t=micros();if(job.received&&(uint32_t)(t-job.lastRxUs)>t15us(busBaud)){completed(Error::Gap);while(rs485.available())rs485.read();return;}
   if(job.received==sizeof(job.response)){completed(Error::Overflow);return;}
   job.response[job.received++]=rs485.read();job.lastRxUs=t;lastActivityUs=t;
  }
  // Décider seulement après un silence de fin de trame ; octets supplémentaires refusés.
  if(job.received&&(uint32_t)(micros()-job.lastRxUs)>=t35us(busBaud)){uint8_t ex=0;Error e=checkResponse(job.response,job.received,job.req,ex);job.exception=ex;completed(e);return;}
  if(nowMs-job.sentMs>=config["responseTimeoutMs"].as<uint32_t>())completed(Error::Timeout);
  return;
 }
 // Les octets tardifs ne peuvent pas devenir la réponse de la transaction suivante.
 while(rs485.available()){rs485.read();lastActivityUs=micros();}
 if(nowMs<nextBus||(uint32_t)(micros()-lastActivityUs)<t35us(busBaud))return;
 if(job.queued){
  if(busBaud!=job.baud){rs485.end();busBaud=job.baud;rs485.begin(busBaud,SERIAL_8N1,hardware::rs485Rx,hardware::rs485Tx);lastActivityUs=micros();return;}
  if(hardware::directionPin>=0)digitalWrite(hardware::directionPin,hardware::directionActiveHigh?HIGH:LOW);
  rs485.write(job.req.data(),job.req.size());job.txEndUs=micros()+(80000000UL+busBaud-1)/busBaud;job.sentMs=nowMs;job.queued=false;job.pending=true;diag.lastTx=hex(job.req.data(),8);diag.status="Transaction en cours";return;
 }
 if(!config["busCommissioned"].as<bool>()||nowMs<nextPoll)return;
 auto s=config["sensors"][pollSensor==0?"ts":"nh"];bool combined=pollSensor==1&&s["combined"].as<bool>();queueRead(pollSensor,s["address"],s["baud"],pollSensor==0||combined?0:2,pollSensor==0||combined?3:1);
 pollSensor++;if(pollSensor==2){pollSensor=0;nextPoll=nowMs+config["pollMs"].as<uint32_t>();}
}
void applyOutputs(){
 if(!config["hardwareConfirmed"].as<bool>()){ioOk=false;for(auto& r:ctl.relays)r.writeAcknowledged=false;return;}
 if(!ioStarted){Wire.begin(hardware::i2cSda,hardware::i2cScl);Wire.setTimeOut(25);ioStarted=true;}
 uint8_t mask=0xff;for(int i=0;i<6;i++)if(ctl.relays[i].command&&ctl.config.relays[i].confirmed&&ctl.config.relays[i].policyConfirmed){if(hardware::relayActiveLow)mask&=~(1<<hardware::relayBits[i]);else mask|=1<<hardware::relayBits[i];}
 if(mask!=lastOutput||!ioOk){Wire.beginTransmission(hardware::relayAddress);Wire.write(mask);bool ack=Wire.endTransmission()==0;if(ack!=ioOk)logEvent("I2C",ack?"Écriture acquittée (équipement non vérifié)":"Échec écriture relais ; état physique inconnu");ioOk=ack;if(ack)lastOutput=mask;}
 for(auto& r:ctl.relays)r.writeAcknowledged=ioOk;
}
void controlTask(void*){
 for(;;){
  {Guard guard;uint32_t clock=millis();nowMs+=(uint32_t)(clock-lastClock);lastClock=clock;
   if(hardware::directionPin>=0&&job.pending&&(int32_t)(micros()-job.txEndUs)>=0)digitalWrite(hardware::directionPin,hardware::directionActiveHigh?LOW:HIGH);
   busTick();bool before[6];for(int i=0;i<6;i++)before[i]=ctl.relays[i].command;ctl.network=WiFi.status()==WL_CONNECTED;ctl.tick(nowMs);applyOutputs();
   for(int i=0;i<6;i++)if(before[i]!=ctl.relays[i].command)logEvent("relais",String("R")+String(i+1)+" commande "+(ctl.relays[i].command?"ON":"OFF")+" / "+reasonName(ctl.relays[i].reason));
   bool bad=ctl.fault(nowMs);String alarm=bad?"Mesure invalide : politique de défaut":ctl.boostDemand()?"Consigne dépassée":"";if(ioStarted&&!ioOk)alarm="Défaut I2C : état physique des sorties inconnu";if(alarm!=lastAlarm){logEvent("alarme",alarm.length()?alarm:"Retour à la normale");lastAlarm=alarm;}
   if(nowMs-lastSample>=config["pollMs"].as<uint32_t>()){lastSample=nowMs;auto& sample=samples[sampleHead];sample.time=nowMs;sample.valid=0;for(int i=0;i<4;i++){sample.v[i]=ctl.measures[i].value;if(ctl.measures[i].valid(nowMs,ctl.config.staleMs))sample.valid|=1<<i;}sampleHead=(sampleHead+1)%360;if(sampleUsed<360)sampleUsed++;}
  }
  vTaskDelay(pdMS_TO_TICKS(1));
 }
}
bool authenticate(){if(!password.length()){server.send(503,"text/plain","Configurer AUTH par USB série. Voir README.");return false;}if(!server.authenticate(userName.c_str(),password.c_str())){server.requestAuthentication(DIGEST_AUTH,"Serre avicole","Authentification requise");return false;}server.sendHeader("Cache-Control","no-store");server.sendHeader("X-Content-Type-Options","nosniff");return true;}
bool writeAuth(){if(!authenticate())return false;if(server.header("X-CSRF-Token")!=csrf){server.send(403,"text/plain","Jeton invalide ; recharger la page");return false;}return true;}
void sendJson(JsonDocument& d){String text;serializeJson(d,text);server.send(200,"application/json; charset=utf-8",text);}
bool body(JsonDocument& d){if(server.arg("plain").length()>16000||deserializeJson(d,server.arg("plain"))){server.send(400,"text/plain","JSON invalide ou trop volumineux");return false;}return true;}
void state(){
 if(!authenticate())return;JsonDocument d;
 {Guard guard;d["simulation"]=false;d["mode"]=ctl.mode==Mode::Observation?"observation":ctl.mode==Mode::Manual?"manual":"automatic";d["network"]=ctl.network;d["uptimeMs"]=nowMs;d["alarm"]=lastAlarm;bool ready=warm.ready(nowMs,config["sensors"]["nh"]["rangePpm"]);d["warmup"]["started"]=warm.started;d["warmup"]["ready"]=ready;if(ready)d["warmup"]["remainingMs"]=0;else if(warm.started)d["warmup"]["remainingMs"]=nhWarmupMs(config["sensors"]["nh"]["rangePpm"])-(nowMs-warm.start);else d["warmup"]["remainingMs"]=nullptr;
  auto a=d["values"].to<JsonArray>();const char* names[]={"Température","Humidité","CO₂","NH₃"};const char* units[]={"°C","%RH","ppm","ppm"};for(int i=0;i<4;i++){auto o=a.add<JsonObject>();auto& m=ctl.measures[i];o["name"]=names[i];o["unit"]=units[i];if(std::isfinite(m.value))o["value"]=m.value;else o["value"]=nullptr;o["valid"]=m.valid(nowMs,ctl.config.staleMs);if(m.seen)o["ageMs"]=nowMs-m.last;else o["ageMs"]=nullptr;o["status"]=errorName(m.error==Error::Ok&&!m.valid(nowMs,ctl.config.staleMs)?Error::Stale:m.error);}
  auto r=d["relays"].to<JsonArray>();for(int i=0;i<6;i++){auto o=r.add<JsonObject>();o["name"]=config["relays"][i]["name"];o["role"]=config["relays"][i]["role"];o["confirmed"]=ctl.config.relays[i].confirmed;o["command"]=ctl.relays[i].command;o["reason"]=reasonName(ctl.relays[i].reason);o["writeAcknowledged"]=ctl.relays[i].writeAcknowledged;o["physicalState"]=nullptr;o["mode"]=d["mode"];}
  auto x=d["diagnostic"].to<JsonObject>();x["hardwareConfirmed"]=config["hardwareConfirmed"];x["busCommissioned"]=config["busCommissioned"];x["success"]=diag.success;x["timeout"]=diag.timeout;x["crc"]=diag.crc;x["exceptions"]=diag.exception;x["lastTx"]=diag.lastTx;x["lastRx"]=diag.lastRx;x["status"]=diag.status;x["exceptionCode"]=job.exception;x["commissionDone"]=diag.commissionDone;x["commissionVerified"]=diag.commissionVerified;x["nhRestartRequired"]=diag.nhRestartRequired;
 }sendJson(d);
}
void history(){if(!authenticate())return;JsonDocument d;{Guard guard;auto a=d["samples"].to<JsonArray>();for(unsigned n=0;n<sampleUsed;n++){auto& s=samples[(sampleHead+360-sampleUsed+n)%360];auto o=a.add<JsonObject>();o["t"]=s.time;auto v=o["v"].to<JsonArray>();for(int i=0;i<4;i++)if(s.valid&(1<<i))v.add(s.v[i]);else v.add(nullptr);}auto ev=d["events"].to<JsonArray>();for(unsigned n=0;n<eventUsed;n++){auto& e=events[(eventHead+128-eventUsed+n)%128];auto o=ev.add<JsonObject>();o["t"]=e.time;o["type"]=e.type;o["detail"]=e.detail;}}sendJson(d);}
void commission(){
 if(!writeAuth())return;JsonDocument d;if(!body(d))return;String sensor=d["sensor"].as<String>(),action=d["action"].as<String>();String err;
 {Guard guard;
  bool energized=false;for(auto& r:ctl.relays)energized|=r.command;
  if(ctl.mode!=Mode::Observation||energized||!d["isolated"].as<bool>()||!config["hardwareConfirmed"].as<bool>())err="Observation, sorties au repos, matériel confirmé et un seul capteur connecté requis";
  else if(job.pending||job.queued)err="Transaction en cours ; réessayer après son achèvement";
  else if((sensor!="ts"&&sensor!="nh")||!settings::uintIn(d["currentAddress"],1,247)||!d["currentBaud"].is<int>()||!settings::rate(d["currentBaud"]))err="Capteur ou paramètres actuels invalides";
  else if(action!="read-current"&&action!="set-address"&&action!="set-baud"&&action!="verify")err="Commande de mise en service inconnue";
  else if(sensor=="nh"&&action=="verify"&&diag.nhRestartRequired&&!d["powerRestartConfirmed"].as<bool>())err="Confirmer le redémarrage électrique NH3 avant relecture";
  else{
   bool ts=sensor=="ts";uint8_t address=d["currentAddress"];uint32_t baud=d["currentBaud"];uint16_t reg=ts?0x4000:0x07d0;uint16_t value=2;uint8_t fn=3;
   if(action=="set-address"){fn=6;value=ts?1:2;}if(action=="set-baud"){fn=6;reg++;value=ts?3:2;}if(action=="verify"){address=ts?1:2;baud=9600;}
   diag.commissionDone=false;diag.commissionVerified=false;diag.commissionSensor=sensor;diag.commissionAction=action;config["busCommissioned"]=false;for(auto& m:ctl.measures)m.error=Error::Unconfirmed;
   if(!ts&&fn==6)diag.nhRestartRequired=true;queueRead(2,address,baud,reg,value,fn);logEvent("mise en service","Commande explicite "+sensor+" / "+action);
  }
 }
 if(err.length())server.send(400,"text/plain",err);else server.send(202,"text/plain","Commande lancée ; consulter le diagnostic. Pas de répétition automatique.");
}
void serialAuth(){
 static String line;while(Serial.available()){char ch=Serial.read();if(ch=='\r')continue;if(ch!='\n'){if(line.length()<200)line+=ch;continue;}
 if(line.startsWith("AUTH ")){int a=line.indexOf(' ',5),b=line.indexOf(' ',a+1);String u=line.substring(5,a),p=line.substring(a+1,b),ap=line.substring(b+1);if(a>5&&b>a&&u.length()<=32&&p.length()>=16&&p.length()<=64&&ap.length()>=12&&ap.length()<=63){userName=u;password=p;apPassword=ap;prefs.putString("user",u);prefs.putString("password",p);prefs.putString("apPassword",ap);WiFi.softAP("Serre-A6v3",ap.c_str());Serial.println("Authentification enregistrée ; secret non affiché.");}else Serial.println("AUTH utilisateur mot_de_passe_16_caracteres_min mot_de_passe_AP_12_caracteres_min");}
 line="";
 }
}
void setup(){
 Serial.begin(115200);mutex=xSemaphoreCreateMutex();prefs.begin("avicole",false);Config parsed;
 String saved=prefs.getString("config",DEFAULT_CONFIG);if(deserializeJson(config,saved)||settings::parse(config,parsed).length()){deserializeJson(config,DEFAULT_CONFIG);settings::parse(config,parsed);}ctl.configure(parsed,0);warm=Warmup{};
 userName=prefs.getString("user","");password=prefs.getString("password","");apPassword=prefs.getString("apPassword","");if(!apPassword.length()){apPassword=String(esp_random(),HEX)+String(esp_random(),HEX)+String(esp_random(),HEX);prefs.putString("apPassword",apPassword);Serial.println("Mot de passe initial du point d'accès : "+apPassword);}csrf=String(esp_random(),HEX)+String(esp_random(),HEX)+String(esp_random(),HEX);
 WiFi.mode(WIFI_AP_STA);WiFi.softAP("Serre-A6v3",apPassword.c_str());String ssid=prefs.getString("ssid","");if(ssid.length())WiFi.begin(ssid.c_str(),prefs.getString("wifiPass","").c_str());
 if(hardware::directionPin>=0){pinMode(hardware::directionPin,OUTPUT);digitalWrite(hardware::directionPin,hardware::directionActiveHigh?LOW:HIGH);}busBaud=config["sensors"]["ts"]["baud"];rs485.begin(busBaud,SERIAL_8N1,hardware::rs485Rx,hardware::rs485Tx);
 const char* headers[]={"X-CSRF-Token"};server.collectHeaders(headers,1);
 server.on("/",HTTP_GET,[](){if(authenticate())server.send_P(200,"text/html; charset=utf-8",PAGE);});server.on("/api/state",HTTP_GET,state);server.on("/api/history",HTTP_GET,history);
 server.on("/api/config",HTTP_GET,[](){if(!authenticate())return;JsonDocument d;{Guard g;d["config"]=config.as<JsonVariant>();d["csrf"]=csrf;}sendJson(d);});
 server.on("/api/config",HTTP_POST,[](){if(!writeAuth())return;JsonDocument d;if(!body(d))return;Config c;String e=settings::parse(d,c);if(e.length()){server.send(400,"text/plain",e);return;}String encoded;serializeJson(d,encoded);bool busy;{Guard g;busy=job.pending||job.queued;if(!busy){config.clear();config.set(d.as<JsonVariant>());ctl.configure(c,nowMs);warm=Warmup{};sampleHead=sampleUsed=0;nextPoll=nowMs;pollSensor=0;logEvent("configuration","Réglages enregistrés ; observation et préchauffage à attester");}}if(busy){server.send(409,"text/plain","Attendre la fin de transaction");return;}prefs.putString("config",encoded);server.send(200,"text/plain","Enregistré. Réarmer le mode et attester le préchauffage.");});
 server.on("/api/mode",HTTP_POST,[](){if(!writeAuth())return;JsonDocument d;if(!body(d))return;String m=d["mode"].as<String>();bool ok=false;{Guard g;if(m=="observation"||m=="manual"||m=="automatic"){ok=ctl.setMode(m=="manual"?Mode::Manual:m=="automatic"?Mode::Automatic:Mode::Observation);if(ok)logEvent("mode",m);}}server.send(ok?200:400,"text/plain",ok?"Mode changé":"Mode inconnu ou confirmations requises");});
 server.on("/api/relay",HTTP_POST,[](){if(!writeAuth())return;JsonDocument d;if(!body(d))return;bool ok=false;{Guard g;if(settings::uintIn(d["index"],0,5)&&d["on"].is<bool>())ok=ctl.manualCommand(d["index"],d["on"]);}server.send(ok?200:400,"text/plain",ok?"Demande enregistrée ; arbitrage applicable":"Mode manuel et relais confirmé requis");});
 server.on("/api/pulse",HTTP_POST,[](){if(!writeAuth())return;JsonDocument d;if(!body(d))return;bool ok=false;{Guard g;if(ctl.mode==Mode::Observation&&d["benchConfirmed"].as<bool>()&&config["hardwareConfirmed"].as<bool>()&&settings::uintIn(d["index"],0,5)&&settings::uintIn(d["durationMs"],100,2000)){ok=ctl.pulse(d["index"],d["durationMs"],nowMs);if(ok)logEvent("test relais","Impulsion limitée, banc attesté");}}server.send(ok?200:400,"text/plain",ok?"Impulsion lancée":"Banc, matériel et relais confirmés, sorties au repos requis");});
 server.on("/api/warmup",HTTP_POST,[](){if(!writeAuth())return;JsonDocument d;if(!body(d))return;String a=d["action"].as<String>();bool ok=a=="start"||(a=="ready"&&d["attested"].as<bool>());{Guard g;if(ok){if(a=="start")warm.restart(nowMs);else warm.explicitlyReady=true;ctl.measures[3].error=Error::Warming;logEvent("préchauffage",a);}}server.send(ok?200:400,"text/plain",ok?"Préchauffage mis à jour":"Attestation requise");});
 server.on("/api/commission",HTTP_POST,commission);
 server.on("/api/wifi",HTTP_POST,[](){if(!writeAuth())return;JsonDocument d;if(!body(d))return;String s=d["ssid"].as<String>(),p=d["password"].as<String>();if(s.length()<1||s.length()>32||p.length()>63||(p.length()>0&&p.length()<8)){server.send(400,"text/plain","Wi-Fi invalide");return;}prefs.putString("ssid",s);prefs.putString("wifiPass",p);WiFi.begin(s.c_str(),p.c_str());server.send(200,"text/plain","Connexion demandée ; le contrôle local continue");});
 server.onNotFound([](){server.send(404,"text/plain","Route inconnue");});server.begin();logEvent("démarrage","A6v3 ; observation ; aucune sortie non confirmée");xTaskCreatePinnedToCore(controlTask,"avicole",12288,nullptr,2,nullptr,1);Serial.println("Point d'accès Serre-A6v3, http://192.168.4.1 ; provisionner AUTH par USB.");
}
void loop(){server.handleClient();serialAuth();delay(1);}