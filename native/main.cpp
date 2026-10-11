#include "control.h"
#include <iostream>
#include <sstream>
#include <string>
using namespace avicole;
int main(){
 Controller c;Config cfg;cfg.validated=true;Role roles[]={Role::VentMain,Role::VentExtra,Role::Heating,Role::Humidifier,Role::Lighting,Role::Alarm};
 for(int i=0;i<6;i++){cfg.relays[i].role=roles[i];cfg.relays[i].confirmed=cfg.relays[i].policyConfirmed=true;cfg.relays[i].startup=(i==0||i==5)?Policy::On:Policy::Off;cfg.relays[i].sensorFault=(i<2||i==5)?Policy::On:Policy::Off;}
 c.configure(cfg,0);std::string line;
 while(std::getline(std::cin,line)){std::istringstream in(line);uint64_t t;float v[4];int mask,network,mode,reboot;if(!(in>>t>>v[0]>>v[1]>>v[2]>>v[3]>>mask>>network>>mode>>reboot))return 2;
  if(reboot)c.restart(t);else c.setMode(Mode(mode));c.network=network;
  if(mask>=0)for(int i=0;i<4;i++){if(mask&(1<<i))c.measures[i].accept(v[i],t);else c.measures[i].error=Error::Timeout;}
  c.tick(t);std::cout<<"{\"commands\":[";for(int i=0;i<6;i++){if(i)std::cout<<',';std::cout<<(c.relays[i].command?"true":"false");}std::cout<<"],\"mode\":"<<int(c.mode)<<",\"fault\":"<<(c.fault(t)?"true":"false")<<"}"<<std::endl;
 }
}