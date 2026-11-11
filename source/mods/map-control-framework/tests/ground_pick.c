/* Exercise the actual native conversion boundary with a scaled/translated
 * overlay and mocked runtime-invoke boxes; no renderer or synthetic pick claim. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef void *PTR;
typedef unsigned int DWORD;
typedef struct {float x,y,z;} DVec;
static int vector2_class,vector3_class,other_class,overlay_field,overlay,map_object,field_type_object;
static int transform_method,screen_method,pick_method;
static int missing_overlay,bad_field_type,bad_offset,fail_stage,bad_class_stage,bad_value_stage,empty_pick,calls;
static DVec seen_local,seen_world;static float seen_screen[2];
typedef struct {PTR klass;DVec value;} Box;
static Box boxes[3];
static PTR find_class(const char *ns,const char *name){assert(!strcmp(ns,"UnityEngine")||!strcmp(ns,"MoonCleaners"));return !strcmp(name,"Vector2")?&vector2_class:!strcmp(name,"Vector3")?&vector3_class:&map_object;}
static PTR get_field(PTR cls,const char *name){assert(cls==&map_object&&!strcmp(name,"overlayLayer"));return &overlay_field;}
static PTR field_type(PTR field){assert(field==&overlay_field);return &field_type_object;}
static int field_offset(PTR field){assert(field==&overlay_field);return bad_offset?632:640;}
static PTR GetProcAddress(PTR module,const char *name){(void)module;if(!strcmp(name,"il2cpp_field_get_type"))return (PTR)field_type;if(!strcmp(name,"il2cpp_field_get_offset"))return (PTR)field_offset;return 0;}
static PTR dmethod(const char *ns,const char *kind,const char *name,const char *signature){
 if(!strcmp(name,"TransformPoint")){assert(!strcmp(ns,"UnityEngine")&&!strcmp(kind,"Transform")&&!strcmp(signature,"UnityEngine.Vector3"));return &transform_method;}
 if(!strcmp(name,"WorldToScreenPoint")){assert(!strcmp(ns,"UnityEngine")&&!strcmp(kind,"RectTransformUtility")&&!strcmp(signature,"UnityEngine.Camera;UnityEngine.Vector3"));return &screen_method;}
 assert(!strcmp(ns,"MoonCleaners")&&!strcmp(kind,"MoonMapScreen")&&!strcmp(name,"Pick")&&!strcmp(signature,"UnityEngine.Vector2"));return &pick_method;
}
static DWORD method_token(PTR method){return method==&transform_method?100667384:method==&screen_method?100663387:100668218;}
static char *type_name(PTR type){assert(type==&field_type_object);return bad_field_type?"UnityEngine.Vector2":"UnityEngine.RectTransform";}
static void type_free(PTR name){(void)name;}
static struct {char *(*type_name)(PTR);void (*free)(PTR);} da={type_name,type_free};
static int deq(const char *a,const char *b){return !strcmp(a,b);}
static void field_value(PTR map,PTR field,void *out){assert(map==&map_object&&field==&overlay_field);*(PTR*)out=missing_overlay?0:&overlay;}
static PTR object_class(PTR boxed){return ((Box*)boxed)->klass;}
static PTR unbox(PTR boxed){return &((Box*)boxed)->value;}
static float invalid_float(void){union {unsigned bits;float value;} value={0x7fc00000};return value.value;}
static PTR call(PTR method,PTR self,void **args,int *ok){
 calls++;int stage=method==&transform_method?1:method==&screen_method?2:3;assert(calls==stage);
 if(fail_stage==stage){*ok=0;return 0;}if(stage==3&&empty_pick)return 0;
 Box *box=boxes+stage-1;box->klass=stage==2?&vector2_class:&vector3_class;if(bad_class_stage==stage)box->klass=&other_class;
 if(stage==1){assert(self==&overlay);seen_local=*(DVec*)args[0];assert(seen_local.z==0);box->value=(DVec){seen_local.x*2+640,seen_local.y*2+360,0};}
 else if(stage==2){assert(!self&&!args[0]);seen_world=*(DVec*)args[1];box->value=seen_world;}
 else{assert(self==&map_object);float *screen=args[0];seen_screen[0]=screen[0];seen_screen[1]=screen[1];box->value=(DVec){.4f,.5f,.7f};}
 if(bad_value_stage==stage)box->value.x=invalid_float();return box;
}
#include "../ground_pick.h"
static void reset(void){missing_overlay=bad_field_type=bad_offset=fail_stage=bad_class_stage=bad_value_stage=empty_pick=calls=0;memset(boxes,0,sizeof(boxes));}
int main(void){
 MMCGroundBindings binding={0};reset();assert(mmc_ground_bind(&binding,0));bad_field_type=1;assert(!mmc_ground_bind(&binding,0));bad_field_type=0;bad_offset=1;assert(!mmc_ground_bind(&binding,0));reset();assert(mmc_ground_bind(&binding,0));
 DVec result={9,9,9};MMCGroundV2 screen={0};assert(mmc_ground_pick_local(&binding,&map_object,(MMCGroundV2){-10,-20},&result,&screen)==MMC_GROUND_OK);
 assert(calls==3&&seen_local.x==-10&&seen_local.y==-20&&seen_world.x==620&&seen_world.y==320&&seen_screen[0]==620&&seen_screen[1]==320);assert(screen.x==620&&screen.y==320&&result.x==.4f&&result.y==.5f&&result.z==.7f);
 reset();assert(mmc_ground_pick_local(&binding,&map_object,(MMCGroundV2){invalid_float(),0},&result,0)==MMC_GROUND_LOCAL&&calls==0);
 reset();missing_overlay=1;assert(mmc_ground_pick_local(&binding,&map_object,(MMCGroundV2){0,0},&result,0)==MMC_GROUND_OVERLAY&&calls==0);
 const int expected[]={0,MMC_GROUND_TRANSFORM,MMC_GROUND_SCREEN,MMC_GROUND_PICK};
 for(int stage=1;stage<=3;stage++){
  reset();fail_stage=stage;result=(DVec){9,9,9};assert(mmc_ground_pick_local(&binding,&map_object,(MMCGroundV2){0,0},&result,0)==expected[stage]);assert(calls==stage&&result.x==9);
  reset();bad_class_stage=stage;assert(mmc_ground_pick_local(&binding,&map_object,(MMCGroundV2){0,0},&result,0)==expected[stage]&&calls==stage);
  reset();bad_value_stage=stage;assert(mmc_ground_pick_local(&binding,&map_object,(MMCGroundV2){0,0},&result,0)==expected[stage]&&calls==stage);
 }
 reset();empty_pick=1;assert(mmc_ground_pick_local(&binding,&map_object,(MMCGroundV2){0,0},&result,0)==MMC_GROUND_PICK&&calls==3);
 reset();assert(mmc_ground_pick_local(&binding,&map_object,(MMCGroundV2){-1000,0},&result,0)==MMC_GROUND_SCREEN&&calls==2);
 assert(!strcmp(mmc_ground_failure_event(MMC_GROUND_TRANSFORM),"ground-pick-transform-failed"));
 puts("production native overlay inverse, null camera, pixel Pick and invalid conversion gates passed");return 0;
}
