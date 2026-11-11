#ifndef MOON_MAP_CONTROL_REGISTRY_H
#define MOON_MAP_CONTROL_REGISTRY_H
#include "sdk/map_controls.h"
typedef struct {
 MoonMapProvider definition;
 char key[64],title[64];
 MoonMapRow row;
 int selected,visible;
} MMCEntry;
typedef struct {MMCEntry entries[MMC_PROVIDER_LIMIT];unsigned count;} MMCRegistry;
static unsigned mmc_length(const char *s,unsigned cap){unsigned n=0;if(!s)return cap;while(n<cap&&s[n])n++;return n;}
static int mmc_equal(const char *a,const char *b){if(!a||!b)return a==b;while(*a&&*a==*b){a++;b++;}return *a==*b;}
static void mmc_copy(char *dst,const char *src,unsigned cap){unsigned n=0;while(n+1<cap&&src[n]){dst[n]=src[n];n++;}dst[n]=0;}
static int mmc_register(MMCRegistry *r,const MoonMapProvider *d){
 if(!d||d->abi!=MMC_ABI||d->size!=sizeof(*d)||!d->describe||!d->invoke||!d->key||!d->key[0]||mmc_length(d->key,64)>=64||!d->title||!d->title[0]||mmc_length(d->title,64)>=64||r->count>=MMC_PROVIDER_LIMIT)return 0;
 for(unsigned i=0;i<r->count;i++)if(mmc_equal(r->entries[i].key,d->key))return 0;
 MMCEntry *e=r->entries+r->count++;e->definition=*d;mmc_copy(e->key,d->key,64);mmc_copy(e->title,d->title,64);e->definition.key=e->key;e->definition.title=e->title;e->selected=-1;return 1;
}
static int mmc_row_valid(const MoonMapRow *r){
 if(r->unit_count>MMC_UNIT_LIMIT||r->action_count>MMC_ACTION_LIMIT||mmc_length(r->status,MMC_TEXT_BYTES)>=MMC_TEXT_BYTES)return 0;
 int selected=r->selected_id==-1&&!r->unit_count;
 for(unsigned i=0;i<r->unit_count;i++){
  if(r->units[i].id<0||!r->units[i].label[0]||mmc_length(r->units[i].label,64)>=64)return 0;
  if(r->units[i].id==r->selected_id)selected=1;
  for(unsigned j=0;j<i;j++)if(r->units[j].id==r->units[i].id)return 0;
 }
 for(unsigned i=0;i<r->action_count;i++){
  if(!r->actions[i].command||!r->actions[i].label[0]||mmc_length(r->actions[i].label,32)>=32||(r->actions[i].enabled!=0&&r->actions[i].enabled!=1))return 0;
  for(unsigned j=0;j<i;j++)if(r->actions[i].command==r->actions[j].command)return 0;
 }
 return selected;
}
static int mmc_cycle(MMCEntry *e,int step){
 if(!e->row.unit_count)return 0;unsigned at=0;
 for(unsigned i=0;i<e->row.unit_count;i++)if(e->row.units[i].id==e->selected){at=i;break;}
 at=(at+e->row.unit_count+(step<0?-1:1))%e->row.unit_count;e->selected=e->row.units[at].id;return 1;
}
typedef struct {float x,y,w,h;} MMCRect;
static int mmc_hit(MMCRect r,float x,float y){return x>=r.x&&y>=r.y&&x<r.x+r.w&&y<r.y+r.h;}
typedef struct {int provider,control,drag,prior_left;} MMCPress;
typedef struct {int captured,provider,control;} MMCPressResult;
static MMCPressResult mmc_press_step(MMCPress *s,int over,int left,int provider,int control){
 MMCPressResult result={over||s->drag,-1,-1};
 if(left&&!s->prior_left){s->provider=s->control=-1;if(over){s->drag=1;s->provider=provider;s->control=control;result.captured=1;}}
 if(!left&&s->prior_left){
  if(s->drag&&provider>=0&&provider==s->provider&&control==s->control){result.provider=provider;result.control=control;}
  s->drag=0;s->provider=s->control=-1;
 }
 s->prior_left=left;return result;
}
#endif
