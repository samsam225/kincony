#include "control.h"
#include <iostream>
#include <cstdlib>
using namespace avicole;
int checks=0;
#define CHECK(x) do {++checks;if(!(x)){std::cerr<<"ÉCHEC ligne "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
static bool close(float a,float b){return std::abs(a-b)<0.001f;}
static void fresh(Controller& c,uint64_t now,float t=25,float h=60,float co=500,float nh=5){float v[]={t,h,co,nh};for(int i=0;i<4;i++)c.measures[i].accept(v[i],now);}
int main(){
 auto req=request(1,3,0,3);CHECK(req[6]==0x05&&req[7]==0xcb);
 uint8_t ts[]={1,3,6,0x01,0x11,0x02,0x37,0x01,0xa7,0x2c,0xfb};uint8_t ex=0;CHECK(checkResponse(ts,sizeof(ts),req,ex)==Error::Ok);auto d=decodeTs(ts+3);CHECK(close(d.temperature,27.3));CHECK(close(d.humidity,56.7));CHECK(d.co2==423);
 uint8_t negative[]={0xff,0x9b,0,0,0,0};CHECK(close(decodeTs(negative).temperature,-10.1));
 uint8_t nh[]={0,0x64};CHECK(close(decodeNh(nh,50,false).nh3,10));for(int r:{100,500,1000})CHECK(decodeNh(nh,r,false).nh3==100);
 uint8_t combined[]={0x01,0x67,0xff,0xb5,0,0x64};auto n=decodeNh(combined,500,true);CHECK(close(n.humidity,35.9));CHECK(close(n.temperature,-7.5));CHECK(n.nh3==100);
 auto nr=request(1,3,2,1);CHECK(nr[6]==0x25&&nr[7]==0xca);uint8_t response[]={1,3,2,0,0x64,0xb9,0xaf};CHECK(checkResponse(response,7,nr,ex)==Error::Ok);response[5]=0x9b;CHECK(checkResponse(response,7,nr,ex)==Error::Crc);
 auto write=request(1,6,0x07d0,2);CHECK(write[6]==0x08&&write[7]==0x86);CHECK(checkResponse(write.data(),8,write,ex)==Error::Ok);
 uint8_t exception[]={1,0x83,2,0xc0,0xf1};CHECK(checkResponse(exception,5,nr,ex)==Error::Exception);CHECK(ex==2);CHECK(checkResponse(ts,4,req,ex)==Error::Length);
 response[0]=2;response[5]=crc16(response,5)&255;response[6]=crc16(response,5)>>8;CHECK(checkResponse(response,7,nr,ex)==Error::Address);response[0]=1;response[1]=4;auto crc=crc16(response,5);response[5]=crc&255;response[6]=crc>>8;CHECK(checkResponse(response,7,nr,ex)==Error::Function);
 CHECK(t35us(9600)>=4000);CHECK(t15us(9600)>=1700);CHECK(t35us(38400)==1750);CHECK(nhWarmupMs(500)==172800000);CHECK(nhWarmupMs(50)==300000);
 Warmup w;CHECK(!w.ready(172800001,500));w.restart(100);CHECK(!w.ready(300099,50));CHECK(w.ready(300100,50));w.restart(0);CHECK(!w.ready(172799999,500));CHECK(w.ready(172800000,500));w.explicitlyReady=true;CHECK(w.ready(1,500));
 Measurement m;m.accept(3,0);CHECK(m.valid(14999,15000));CHECK(!m.valid(15000,15000));m.error=Error::Timeout;CHECK(!m.valid(1,15000));
 Controller c;Config cfg;cfg.validated=true;cfg.minVentOnMs=0;Role roles[]={Role::VentMain,Role::VentExtra,Role::Heating,Role::Humidifier,Role::Lighting,Role::Alarm};for(int i=0;i<6;i++){cfg.relays[i].role=roles[i];cfg.relays[i].confirmed=cfg.relays[i].policyConfirmed=true;cfg.relays[i].minOnMs=cfg.relays[i].minOffMs=1000;}cfg.relays[0].startup=Policy::On;cfg.relays[0].sensorFault=Policy::On;cfg.relays[1].sensorFault=Policy::On;cfg.relays[5].sensorFault=Policy::On;c.configure(cfg,0);c.tick(0);CHECK(c.relays[0].command);CHECK(c.mode==Mode::Observation);c.setMode(Mode::Automatic);fresh(c,1000,31);c.tick(1000);CHECK(c.relays[0].command&&c.relays[1].command);fresh(c,1500,29.5);c.tick(1500);CHECK(c.relays[1].command);fresh(c,2001,28.9);c.tick(2001);CHECK(!c.relays[1].command);
 fresh(c,2100,19);c.tick(2100);CHECK(c.relays[2].command);fresh(c,2500,21.1);c.tick(2500);CHECK(c.relays[2].command);fresh(c,3101,21.1);c.tick(3101);CHECK(!c.relays[2].command);
 fresh(c,5000,19);c.tick(5000);CHECK(c.relays[2].command);fresh(c,5100,19,60,2000);c.tick(5100);CHECK(!c.relays[2].command);CHECK(c.relays[0].command);CHECK(c.relays[2].reason==Reason::Conflict||c.relays[2].reason==Reason::Automatic);
 c.measures[3].error=Error::Warming;c.tick(5200);CHECK(c.relays[0].command&&c.relays[1].command&&c.relays[5].command);CHECK(!c.relays[2].command);c.network=false;c.tick(5300);CHECK(c.relays[0].command);
 c.restart(5400);c.tick(5400);CHECK(c.mode==Mode::Observation&&c.relays[0].command);CHECK(!c.measures[0].seen);
 c.setMode(Mode::Observation);c.tick(5500);CHECK(!c.relays[0].command);CHECK(c.pulse(4,500,5500));c.tick(5500);CHECK(c.relays[4].command);c.tick(6000);CHECK(!c.relays[4].command);
 cfg.relays[0].confirmed=false;c.configure(cfg,7000);c.tick(7000);CHECK(!c.relays[0].command);CHECK(!c.pulse(0,500,7000));cfg.validated=false;c.configure(cfg,0);CHECK(!c.setMode(Mode::Automatic));
 c.configure(cfg,0);c.setMode(Mode::Manual);CHECK(c.manualCommand(2,true));fresh(c,2000);c.tick(2000);CHECK(c.relays[2].command);CHECK(c.manualCommand(1,true));fresh(c,3000);c.tick(3000);CHECK(c.relays[1].command&&!c.relays[2].command);
 std::cout<<checks<<" assertions C++ réussies\n";
}