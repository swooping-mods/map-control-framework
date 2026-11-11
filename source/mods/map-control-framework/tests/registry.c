#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../registry.h"
#include "../screen_gates.h"
static int describe(const MoonMapContext *ctx,int requested,MoonMapRow *row){(void)ctx;(void)requested;(void)row;return 1;}
static int invoke(const MoonMapContext *ctx,int id,unsigned action){(void)ctx;(void)id;(void)action;return 1;}
static void registration(void){
 MMCRegistry registry={0};char key[64]="gps",title[64]="Vehicles / GPS";MoonMapProvider p={MMC_ABI,sizeof(p),key,title,describe,invoke};
 assert(mmc_register(&registry,&p));strcpy(key,"changed");strcpy(title,"Changed");assert(!strcmp(registry.entries[0].key,"gps"));assert(!strcmp(registry.entries[0].definition.title,"Vehicles / GPS"));assert(registry.entries[0].selected==-1);
 p.key="gps";assert(!mmc_register(&registry,&p));p.key="bad";p.abi++;assert(!mmc_register(&registry,&p));p.abi=MMC_ABI;p.size--;assert(!mmc_register(&registry,&p));p.size=sizeof(p);p.invoke=0;assert(!mmc_register(&registry,&p));p.invoke=invoke;
 char names[MMC_PROVIDER_LIMIT][16];for(unsigned i=1;i<MMC_PROVIDER_LIMIT;i++){snprintf(names[i],sizeof(names[i]),"provider-%u",i);p.key=names[i];assert(mmc_register(&registry,&p));}
 p.key="overflow";assert(!mmc_register(&registry,&p));assert(registry.count==MMC_PROVIDER_LIMIT);
}
static MoonMapRow valid_row(void){
 MoonMapRow row={0};row.unit_count=3;row.selected_id=74;row.units[0].id=12;row.units[1].id=74;row.units[2].id=201;strcpy(row.units[0].label,"Car 12");strcpy(row.units[1].label,"Car 74");strcpy(row.units[2].label,"Car 201");row.action_count=2;row.actions[0]=(MoonMapAction){1,"Drive",1};row.actions[1]=(MoonMapAction){2,"Stop",1};strcpy(row.status,"Ready");return row;
}
static void rows(void){
 MoonMapRow r=valid_row();assert(mmc_row_valid(&r));MMCEntry e={0};e.row=r;e.selected=74;assert(mmc_cycle(&e,1));assert(e.selected==201);assert(mmc_cycle(&e,1));assert(e.selected==12);assert(mmc_cycle(&e,-1));assert(e.selected==201);
 r.units[2].id=74;assert(!mmc_row_valid(&r));r=valid_row();r.selected_id=999;assert(!mmc_row_valid(&r));r=valid_row();r.action_count=MMC_ACTION_LIMIT+1;assert(!mmc_row_valid(&r));r=valid_row();r.unit_count=MMC_UNIT_LIMIT+1;assert(!mmc_row_valid(&r));r=valid_row();r.actions[1].command=1;assert(!mmc_row_valid(&r));r=valid_row();r.actions[1].enabled=2;assert(!mmc_row_valid(&r));r=valid_row();memset(r.status,'x',sizeof(r.status));assert(!mmc_row_valid(&r));
 r=(MoonMapRow){0};r.selected_id=-1;assert(mmc_row_valid(&r));e.row=r;assert(!mmc_cycle(&e,1));
}
static void pointer(void){
 MMCPress press={-1,-1,0,0};MMCPressResult r=mmc_press_step(&press,1,1,0,4);assert(r.captured&&r.provider==-1&&press.drag);r=mmc_press_step(&press,1,0,0,4);assert(r.captured&&r.provider==0&&r.control==4&&!press.drag);
 r=mmc_press_step(&press,1,1,1,2);assert(r.captured);r=mmc_press_step(&press,0,1,-1,-1);assert(r.captured);r=mmc_press_step(&press,0,0,-1,-1);assert(r.captured&&r.provider==-1);r=mmc_press_step(&press,0,0,-1,-1);assert(!r.captured);
 mmc_press_step(&press,0,1,-1,-1);r=mmc_press_step(&press,1,0,1,2);assert(r.captured&&r.provider==-1); /* an outside press cannot fire a panel release */
 mmc_press_step(&press,1,1,0,3);r=mmc_press_step(&press,1,0,0,4);assert(r.provider==-1); /* a different release target cannot fire */
 mmc_press_step(&press,1,1,-1,-1);r=mmc_press_step(&press,1,0,-1,-1);assert(r.captured&&r.provider==-1); /* panel background still consumes */
 assert(mmc_hit((MMCRect){10,20,30,40},10,20));assert(!mmc_hit((MMCRect){10,20,30,40},40,20));assert(!mmc_hit((MMCRect){10,20,30,40},10,60));
}
static void screen_gates(void){
 assert(mmc_screen_input_allowed(1,1,0,0,0,0,0));
 for(unsigned i=0;i<6;i++)assert(!mmc_screen_input_allowed(1,1,0,0,0,0,1u<<i));
 assert(!mmc_screen_input_allowed(0,1,0,0,0,0,0));assert(!mmc_screen_input_allowed(1,0,0,0,0,0,0));
 assert(!mmc_screen_input_allowed(1,1,1,0,0,0,0));assert(!mmc_screen_input_allowed(1,1,0,1,0,0,0));
 assert(!mmc_screen_input_allowed(1,1,0,0,1,0,0));assert(!mmc_screen_input_allowed(1,1,0,0,0,1,0));
}
int main(void){registration();rows();pointer();screen_gates();puts("map controls registry, stable selection, pointer capture and native screen admission passed");return 0;}
