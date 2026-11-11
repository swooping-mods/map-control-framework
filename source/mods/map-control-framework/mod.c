/* One attached map section, with callbacks into real feature providers. */
#include "../../runtime/native/gameplay.h"
#include "../../runtime/native/services.h"
#include "reflection.h"
#include "registry.h"
#include "screen_gates.h"
#include "ground_pick.h"
int _fltused=0;
typedef struct {float x,y;} MMCV2;
_Static_assert(sizeof(DRoot)==sizeof(PTR),"IL2CPP roots are pointer-sized");
typedef struct {DRoot go,rect,image,text;MMCRect hit;char shown[MMC_TEXT_BYTES];int enabled,active,style;} MMCWidget;
static struct {
 int started,last_frame,pointer_over,prior_right,visible,gate_frame,gate_allowed;DWORD thread,registration_thread;
 MMCRegistry registry;MMCGroundBindings ground;MMCPress input;MoonMapContext context;DRoot walker,world,map,panel,panel_rect,canvas;
 MMCWidget title[MMC_PROVIDER_LIMIT],status[MMC_PROVIDER_LIMIT],select[MMC_PROVIDER_LIMIT],previous[MMC_PROVIDER_LIMIT],next[MMC_PROVIDER_LIMIT],actions[MMC_PROVIDER_LIMIT][MMC_ACTION_LIMIT];
 int row_displayed[MMC_PROVIDER_LIMIT];MMCRect bounds;float next_refresh,ground_menu_at,ground_attempt_at;int has_ground,ground_attempted;MoonMapVector direction,point;int sector;
 DRoot screens[6];PTR screen_types[6],screen_open[6],component,pause_menu,pause_blocks,pause_modal;
 PTR alive,world_field,restoring,focused,typing,map_open,map_walker,map_pressing,map_dragging,map_over_panel,map_canvas,map_sector,map_mark,map_menu_open,map_menu_ground,map_menu_spot,map_menu_at;
 PTR get_transform,go_transform,go_ctor,go_add,go_component,parent,destroy,active,anchor_min,anchor_max,pivot,anchored,size,rect;
 PTR color,raycast,text,font,font_size,text_color,text_align,text_wrap,text_overflow,text_rich,font_get,frame,time;
 PTR position_read,mouse,mouse_position,left_button,right_button,button_down,local_point,pick,moon_radius;
 PTR (*return_type)(PTR),(*field_type)(PTR);int (*field_offset)(PTR);void (*field_set)(PTR,PTR,void *);
 DColor native_background,native_foreground;PTR log;
} mc;
static void mc_event(const char *event){if(!mc.log)return;char b[200],*p=cat(b,"{\"event\":\"");p=cat(p,event);p=cat(p,"\"}\n");DWORD written;WriteFile(mc.log,b,(DWORD)(p-b),&written,0);}
static int mc_alive(PTR obj){int ok=1;void *a[]={obj};PTR b=obj?call(mc.alive,0,a,&ok):0;return ok&&b&&*(unsigned char*)unbox(b);}
static void mc_set(PTR method_id,PTR obj,void *value,int *ok){void *a[]={value};if(*ok)call(method_id,obj,a,ok);}
static void mc_active(PTR obj,int visible){if(!mc_alive(obj))return;int ok=1;unsigned char yes=(unsigned char)!!visible;mc_set(mc.active,obj,&yes,&ok);}
static int mc_type(PTR type,const char *expected){if(!type)return 0;char *name=da.type_name(type);int match=name&&deq(name,expected);if(name)da.free(name);return match;}
static PTR mc_add(PTR go,const char *ns,const char *type,int *ok){void *a[]={dtype(ns,type)};if(!a[0]){*ok=0;return 0;}return call(mc.go_add,go,a,ok);}
static PTR mc_go(const char *name,PTR parent,int *ok){
 PTR go=da.new_object(find_class("UnityEngine","GameObject"));if(!go){*ok=0;return 0;}
 void *a[]={da.string(name)};call(mc.go_ctor,go,a,ok);PTR tr=mc_add(go,"UnityEngine","RectTransform",ok);
 if(parent){unsigned char no=0;void *b[]={parent,&no};call(mc.parent,tr,b,ok);}return go;
}
static PTR mc_component(PTR go,const char *ns,const char *type,int *ok){void *a[]={dtype(ns,type)};return a[0]?call(mc.go_component,go,a,ok):0;}
static void mc_rect(PTR rect,MMCRect r,int *ok){
 MMCV2 top={0,1},at={r.x,-r.y},extent={r.w,r.h};mc_set(mc.anchor_min,rect,&top,ok);mc_set(mc.anchor_max,rect,&top,ok);mc_set(mc.pivot,rect,&top,ok);mc_set(mc.anchored,rect,&at,ok);mc_set(mc.size,rect,&extent,ok);
}
static void mc_text(MMCWidget *w,const char *text){
 if(mmc_equal(w->shown,text))return;mmc_copy(w->shown,text,MMC_TEXT_BYTES);int ok=1;void *a[]={da.string(w->shown)};call(mc.text,dget(w->text),a,&ok);
}
static void mc_widget(MMCWidget *w,PTR parent,const char *name,int button,int *ok){
 w->active=w->style=-1;PTR go=mc_go(name,parent,ok);w->go=dhold(go);w->rect=dhold(mc_component(go,"UnityEngine","RectTransform",ok));
 if(button){PTR image=mc_add(go,"UnityEngine.UI","Image",ok);w->image=dhold(image);unsigned char no=0;mc_set(mc.raycast,image,&no,ok);}
 PTR label=button?mc_go("Label",dget(w->rect),ok):go;
 PTR text=mc_add(label,"TMPro","TextMeshProUGUI",ok);w->text=dhold(text);PTR font=call(mc.font_get,0,0,ok);void *args[]={font};if(font)call(mc.font,text,args,ok);else *ok=0;
 float font_size=16;int align=button?514:513,overflow=1;unsigned char no=0;
 mc_set(mc.font_size,text,&font_size,ok);mc_set(mc.text_align,text,&align,ok);mc_set(mc.text_wrap,text,&no,ok);mc_set(mc.text_rich,text,&no,ok);mc_set(mc.text_overflow,text,&overflow,ok);mc_set(mc.raycast,text,&no,ok);
}
static void mc_layout(MMCWidget *w,MMCRect r,int button,int *ok){
 if(w->hit.x==r.x&&w->hit.y==r.y&&w->hit.w==r.w&&w->hit.h==r.h)return;
 w->hit=r;mc_rect(dget(w->rect),r,ok);
 if(button){PTR label_rect=call(mc.get_transform,dget(w->text),0,ok);mc_rect(label_rect,(MMCRect){4,0,r.w-8,r.h},ok);}
}
static void mc_widget_active(MMCWidget *w,int visible){if(w->active==visible)return;w->active=visible;mc_active(dget(w->go),visible);}
static void mc_drop_widget(MMCWidget *w){ddrop(&w->go);ddrop(&w->rect);ddrop(&w->image);ddrop(&w->text);dzero(w,sizeof(*w));}
static void mc_destroy_ui(void){
 PTR panel=dget(mc.panel);if(mc_alive(panel)){int ok=1;void *a[]={panel};call(mc.destroy,0,a,&ok);}ddrop(&mc.panel);ddrop(&mc.panel_rect);ddrop(&mc.canvas);
 for(unsigned i=0;i<MMC_PROVIDER_LIMIT;i++){
  mc_drop_widget(mc.title+i);mc_drop_widget(mc.status+i);mc_drop_widget(mc.select+i);mc_drop_widget(mc.previous+i);mc_drop_widget(mc.next+i);
  for(unsigned j=0;j<MMC_ACTION_LIMIT;j++)mc_drop_widget(mc.actions[i]+j);
 }mc.visible=0;
}
static void mc_reset(void){
 mc_destroy_ui();ddrop(&mc.walker);ddrop(&mc.world);ddrop(&mc.map);mc.ground_attempted=mc.has_ground=mc.input.drag=mc.pointer_over=mc.input.prior_left=mc.prior_right=0;mc.input.provider=mc.input.control=-1;mc.sector=-1;mc.next_refresh=0;mc.ground_menu_at=-1;
 for(unsigned i=0;i<mc.registry.count;i++){mc.registry.entries[i].selected=-1;mc.registry.entries[i].visible=0;}
 for(unsigned i=0;i<6;i++)ddrop(mc.screens+i);mc.gate_frame=-1;mc.gate_allowed=0;
}
static int mc_context(PTR walker,PTR map,PTR world){
 int ok=1;if(!mc_alive(walker)||!mc_alive(map)||!mc_alive(world))return 0;
 int frame=integer(mc.frame,0,&ok);if(!ok)return 0;if(frame==mc.gate_frame)return mc.gate_allowed;
 mc.gate_frame=frame;mc.gate_allowed=0;
 int open=boolean(mc.map_open,map,&ok),focused=boolean(mc.focused,0,&ok),typing=boolean(mc.typing,0,&ok),restoring=boolean(mc.restoring,world,&ok),paused=0,modal=0;unsigned screens=0;
 PTR pause=call(mc.pause_menu,walker,0,&ok);if(mc_alive(pause)){paused=boolean(mc.pause_blocks,pause,&ok);modal=boolean(mc.pause_modal,pause,&ok);}
 if(open)for(unsigned i=0;i<6;i++){
  if(!mc_alive(dget(mc.screens[i]))){ddrop(mc.screens+i);void *a[]={mc.screen_types[i]};mc.screens[i]=dhold(call(mc.component,walker,a,&ok));}
  if(mc_alive(dget(mc.screens[i]))&&boolean(mc.screen_open[i],dget(mc.screens[i]),&ok))screens|=1u<<i;
 }
 mc.gate_allowed=ok&&mmc_screen_input_allowed(open,focused,typing,restoring,paused,modal,screens);return mc.gate_allowed;
}
static int mc_pointer(MMCV2 *point,int *left,int *right){
 int ok=1;PTR mouse=call(mc.mouse,0,0,&ok);if(!ok||!mouse)return 0;DRoot root=dhold(mouse);
 PTR control=call(mc.mouse_position,mouse,0,&ok);if(control&&!mc.position_read){PTR read=get_method(object_class(control),"ReadValue",0);if(read&&mc_type(mc.return_type(read),"UnityEngine.Vector2"))mc.position_read=read;}
 if(!mc.position_read||!control){ddrop(&root);return 0;}
 PTR value=call(mc.position_read,control,0,&ok);if(value)dcopy(point,unbox(value),sizeof(*point));else ok=0;
 PTR l=call(mc.left_button,mouse,0,&ok),r=call(mc.right_button,mouse,0,&ok);
 *left=l?boolean(mc.button_down,l,&ok):0;*right=r?boolean(mc.button_down,r,&ok):0;ddrop(&root);return ok;
}
static float mc_sqrt(float x){float n=x>1?x:1;for(int i=0;i<24;i++)n=.5f*(n+x/n);return n;}
static void mc_ground(void){
 int ok=1;unsigned char menu=0,ground=0;PTR map=dget(mc.map);field_value(map,mc.map_menu_open,&menu);field_value(map,mc.map_menu_ground,&ground);
 if(!menu||!ground)return;float menu_at=0;field_value(map,mc.map_menu_at,&menu_at);if(mc.ground_attempted&&menu_at==mc.ground_attempt_at)return;
 /* One attempt/event per native menu timestamp. Reject a failed new capture
  * instead of exposing an older target as the newly chosen destination. */
 mc.ground_attempted=1;mc.ground_attempt_at=menu_at;mc.has_ground=0;
 MMCGroundV2 spot={0};field_value(map,mc.map_menu_spot,&spot);DVec p={0};int result=mmc_ground_pick_local(&mc.ground,map,spot,&p,0);
 if(result!=MMC_GROUND_OK){mc_event(mmc_ground_failure_event(result));return;}
 float n=mc_sqrt(p.x*p.x+p.y*p.y+p.z*p.z),radius=dnumber(mc.moon_radius,map,&ok);
 if(!ok||radius<=1||radius>1e7f){mc_event("ground-pick-radius-failed");return;}
 mc.direction=(MoonMapVector){p.x/n,p.y/n,p.z/n};mc.point=(MoonMapVector){p.x*radius/n,p.y*radius/n,p.z*radius/n};mc.has_ground=1;mc.ground_menu_at=menu_at;mc_event("ground-picked");
}

static void mc_native_style(void){
 int ok=1;PTR map=dget(mc.map),cls=object_class(map),panels=0,title=0,f=get_field(cls,"panels");if(f)field_value(map,f,&panels);
 if(panels&&da.array_length(panels)){
  PTR panel=*(PTR*)darray(panels,0);void *a[]={dtype("UnityEngine.UI","Graphic")};PTR component=panel?call(dmethod("UnityEngine","Component","GetComponent","System.Type"),panel,a,&ok):0;
  PTR color=component?call(dmethod("UnityEngine.UI","Graphic","get_color",""),component,0,&ok):0;if(ok&&color){dcopy(&mc.native_background,unbox(color),sizeof(DColor));mc.native_background.a=.97f;}
 }
 f=get_field(cls,"infoTitle");if(f)field_value(map,f,&title);PTR color=title?call(dmethod("TMPro","TMP_Text","get_color",""),title,0,&ok):0;
 if(ok&&color)dcopy(&mc.native_foreground,unbox(color),sizeof(DColor));
}
static int mc_ui(PTR canvas){
 if(mc_alive(dget(mc.panel))&&canvas==dget(mc.canvas))return 1;mc_destroy_ui();int ok=1;mc_native_style();
 PTR panel=mc_go("MoonLab map controls",canvas,&ok);mc.panel=dhold(panel);mc.canvas=dhold(canvas);
 PTR image=mc_add(panel,"UnityEngine.UI","Image",&ok);DColor background=mc.native_background;unsigned char no=0;mc_set(mc.color,image,&background,&ok);mc_set(mc.raycast,image,&no,&ok);
 PTR parent=mc_component(panel,"UnityEngine","RectTransform",&ok);mc.panel_rect=dhold(parent);
 for(unsigned i=0;i<mc.registry.count;i++){
  mc_widget(mc.title+i,parent,mc.registry.entries[i].title,0,&ok);mc_widget(mc.status+i,parent,"Status",0,&ok);mc_widget(mc.select+i,parent,"Selected unit",0,&ok);
  mc_widget(mc.previous+i,parent,"Previous unit",1,&ok);mc_widget(mc.next+i,parent,"Next unit",1,&ok);
  for(unsigned j=0;j<MMC_ACTION_LIMIT;j++)mc_widget(mc.actions[i]+j,parent,"Command",1,&ok);
 }
 if(!ok){mc_destroy_ui();mc_event("ui-construction-failed");return 0;}mc_event("native-map-section-created");return 1;
}
static void mc_color_widget(MMCWidget *w,int enabled,int hovered,int focused,int *ok){
 w->enabled=enabled;int style=enabled|(hovered<<1)|(focused<<2);if(w->style==style)return;w->style=style;DColor text=mc.native_foreground;if(!enabled){text.r*=.55f;text.g*=.55f;text.b*=.55f;}mc_set(mc.text_color,dget(w->text),&text,ok);
 if(w->image){float blend=!enabled?.02f:focused?.3f:hovered?.24f:.12f;DColor bg=mc.native_background,fg=mc.native_foreground;bg.r+=(fg.r-bg.r)*blend;bg.g+=(fg.g-bg.g)*blend;bg.b+=(fg.b-bg.b)*blend;bg.a=1;mc_set(mc.color,dget(w->image),&bg,ok);}
}
static void mc_refresh(void){
 for(unsigned i=0;i<mc.registry.count;i++){
  MMCEntry *e=mc.registry.entries+i;dzero(&e->row,sizeof(e->row));e->row.selected_id=-1;
  e->visible=e->definition.describe(&mc.context,e->selected,&e->row)&&mmc_row_valid(&e->row);if(e->visible)e->selected=e->row.selected_id;
 }
}
static void mc_select_name(MMCEntry *e,char *text){
 const char *name="No loaded units";for(unsigned j=0;j<e->row.unit_count;j++)if(e->row.units[j].id==e->selected){name=e->row.units[j].label;break;}
 mmc_copy(text,name,MMC_TEXT_BYTES);
}
static void mc_layout_rows(float width,float height,float px,float py){
 int ok=1;unsigned visible=0;for(unsigned i=0;i<mc.registry.count;i++)if(mc.registry.entries[i].visible)visible++;
 /* Native map uses a 1920-wide canvas with 400-wide side columns. Fit the
  * shared controls between those columns; small canvases get an attached band. */
 float w=width>=1600?width-960:width-32;if(w>1100)w=1100;float x=(width-w)*.5f,y=width>=1600?36:154;
 float h=visible?visible*78.f+8:0;if(y+h>height-100)h=height-y-100;mc.bounds=(MMCRect){x,y,w,h};
 mc_rect(dget(mc.panel_rect),mc.bounds,&ok);mc_active(dget(mc.panel),visible>0);mc.visible=visible>0;
 float row_y=4;
 for(unsigned i=0;i<mc.registry.count;i++){
  MMCEntry *e=mc.registry.entries+i;int shown=e->visible&&row_y+74<=h;mc.row_displayed[i]=shown;
  MMCWidget *base[]={mc.title+i,mc.status+i,mc.select+i,mc.previous+i,mc.next+i};for(unsigned k=0;k<5;k++)mc_widget_active(base[k],shown);
  for(unsigned j=0;j<MMC_ACTION_LIMIT;j++)mc_widget_active(mc.actions[i]+j,shown&&j<e->row.action_count);
  if(!shown)continue;
  mc_layout(mc.title+i,(MMCRect){12,row_y,154,29},0,&ok);mc_text(mc.title+i,e->title);mc_color_widget(mc.title+i,1,0,0,&ok);
  mc_layout(mc.status+i,(MMCRect){170,row_y,w-182,29},0,&ok);mc_text(mc.status+i,e->row.status);mc_color_widget(mc.status+i,1,0,0,&ok);
  float by=row_y+31,selector=w<900?210:254;
  mc_layout(mc.previous+i,(MMCRect){12,by,30,32},1,&ok);mc_text(mc.previous+i,"<");mc_layout(mc.select+i,(MMCRect){48,by,selector-76,32},0,&ok);
  char label[MMC_TEXT_BYTES];mc_select_name(e,label);mc_text(mc.select+i,label);mc_color_widget(mc.select+i,1,0,0,&ok);
  mc_layout(mc.next+i,(MMCRect){selector-24,by,30,32},1,&ok);mc_text(mc.next+i,">");
  MMCWidget *switches[]={mc.previous+i,mc.next+i};for(unsigned k=0;k<2;k++){MMCRect r=switches[k]->hit;r.x+=x;r.y+=y;mc_color_widget(switches[k],e->row.unit_count>1,mmc_hit(r,px,py),mc.input.provider==(int)i&&mc.input.control==(int)k,&ok);}
  float start=selector+16,aw=e->row.action_count?(w-start-12)/e->row.action_count:0;
  for(unsigned j=0;j<e->row.action_count;j++){
   MMCWidget *a=mc.actions[i]+j;mc_layout(a,(MMCRect){start+j*aw,by,aw-6,32},1,&ok);mc_text(a,e->row.actions[j].label);MMCRect r=a->hit;r.x+=x;r.y+=y;
   mc_color_widget(a,e->row.actions[j].enabled,mmc_hit(r,px,py),mc.input.provider==(int)i&&mc.input.control==(int)j+2,&ok);
  }
  row_y+=78;
 }
 if(!ok){mc_active(dget(mc.panel),0);mc.visible=0;mc_event("layout-call-failed");}
}
static int mc_control_at(float x,float y,int *provider,int *control){
 x-=mc.bounds.x;y-=mc.bounds.y;
 for(unsigned i=0;i<mc.registry.count;i++){
  MMCEntry *e=mc.registry.entries+i;if(!e->visible||!mc.row_displayed[i])continue;
  MMCWidget *switches[]={mc.previous+i,mc.next+i};for(unsigned k=0;k<2;k++)if(switches[k]->enabled&&mmc_hit(switches[k]->hit,x,y)){*provider=(int)i;*control=(int)k;return 1;}
  for(unsigned j=0;j<e->row.action_count;j++)if(mc.actions[i][j].enabled&&mmc_hit(mc.actions[i][j].hit,x,y)){*provider=(int)i;*control=(int)j+2;return 1;}
 }return 0;
}
static void mc_invoke(int provider,int control){
 if(provider<0||(unsigned)provider>=mc.registry.count)return;MMCEntry *e=mc.registry.entries+provider;if(!e->visible)return;
 if(control<2){mmc_cycle(e,control?1:-1);mc_event("unit-selected");}
 else{unsigned index=(unsigned)control-2;if(index<e->row.action_count&&e->row.actions[index].enabled){int result=e->definition.invoke(&mc.context,e->selected,e->row.actions[index].command);mc_event(result?"provider-command":"provider-command-rejected");}}
 mc_refresh();
}
static int mc_steer(PTR map,PTR world){
 PTR walker=0;field_value(map,mc.map_walker,&walker);
 if(!mc_alive(walker)||!mc_alive(map)||!mc_alive(world))return 0;
 if(dget(mc.map)!=map||dget(mc.world)!=world||dget(mc.walker)!=walker){mc_reset();mc.map=dhold(map);mc.world=dhold(world);mc.walker=dhold(walker);}
 if(!mc_context(walker,map,world)){mc_active(dget(mc.panel),0);mc.visible=mc.input.drag=mc.pointer_over=0;mc.input.provider=mc.input.control=-1;return 0;}
 int ok=1,frame=integer(mc.frame,0,&ok);float now=dnumber(mc.time,0,&ok);PTR canvas=0;field_value(map,mc.map_canvas,&canvas);if(!ok||!canvas||!mc_ui(canvas))return 0;
 MMCV2 pointer={0};int left=0,right=0;if(!mc_pointer(&pointer,&left,&right))return 0;
 DRect bounds={0};PTR boxed=call(mc.rect,canvas,0,&ok);if(boxed)dcopy(&bounds,unbox(boxed),sizeof(bounds));else ok=0;
 MMCV2 local={0};void *a[]={canvas,&pointer,0,&local};PTR converted=call(mc.local_point,0,a,&ok);if(!ok||!converted||!*(unsigned char*)unbox(converted))return 0;
 float px=local.x-bounds.x,py=bounds.y+bounds.h-local.y;
 /* Reconstruct native ground at its stored screen coordinate. Never treat
  * menuSpot (Vector2) as a world Vector3. */
 if(!(mc.visible&&mmc_hit(mc.bounds,px,py)))mc_ground();
 mc.context=(MoonMapContext){MMC_ABI,sizeof(MoonMapContext),(unsigned)frame,world,walker,map,mc.has_ground,mc.sector,-1,-1,mc.direction,mc.point};
 int hover=-1;field_value(map,mc.map_sector,&hover);if(!mc.visible||!mmc_hit(mc.bounds,px,py)){mc.context.hovered_sector=hover;if(hover>=0)mc.sector=hover;}
 PTR mark=0;field_value(map,mc.map_mark,&mark);if(mark&&(!mc.visible||!mmc_hit(mc.bounds,px,py))){int kind=-1,id=-1;PTR cls=object_class(mark),kindf=get_field(cls,"kind"),idf=get_field(cls,"id");if(kindf&&idf){field_value(mark,kindf,&kind);field_value(mark,idf,&id);if(kind==4)mc.context.hovered_vehicle=id;if(kind==5)mc.context.hovered_robot=id;}}
 if(now>=mc.next_refresh||!mc.visible){mc_refresh();mc.next_refresh=now+.1f;}
 mc_layout_rows(bounds.w,bounds.h,px,py);int hit=mc.visible&&mmc_hit(mc.bounds,px,py);mc.pointer_over=hit;
 int provider=-1,control=-1;mc_control_at(px,py,&provider,&control);
 MMCPressResult press=mmc_press_step(&mc.input,hit,left,provider,control);
 if(press.provider>=0)mc_invoke(press.provider,press.control);
 int captured=press.captured;mc.prior_right=right;
 if(captured){
  /* Steer owns all native pan/select/context-menu input. Cancelling just this
   * method leaves the rest of MoonMapScreen.Update rendering and stats alive.
   * Clear pending presses so release outside cannot select a vanilla marker. */
  unsigned char no=0,yes=1;mc.field_set(map,mc.map_pressing,&no);mc.field_set(map,mc.map_dragging,&no);mc.field_set(map,mc.map_over_panel,&yes);return 1;
 }
 return 0;
}
static int mc_before(int id,Context *context){
 if(!mc.thread)mc.thread=GetCurrentThreadId();if(mc.thread!=GetCurrentThreadId())return 0;
 if(id==1){if(mc_steer((PTR)context->rcx,(PTR)context->rdx)){context->rax=0;return 1;}return 0;}
 PTR walker=(PTR)context->rcx,world=0;field_value(walker,mc.world_field,&world);
 if(dget(mc.world)&&(!mc_alive(world)||world!=dget(mc.world)||walker!=dget(mc.walker))){mc_reset();return 0;}
 if(dget(mc.map)&&!mc_context(walker,dget(mc.map),world)){mc_active(dget(mc.panel),0);mc.visible=0;mc.input.drag=mc.pointer_over=0;mc.input.provider=mc.input.control=-1;int ok=1;if(!boolean(mc.map_open,dget(mc.map),&ok)){mc.has_ground=0;mc.ground_attempted=0;}}
 return 0;
}
static int mc_register_provider(const MoonMapProvider *provider){if(!mc.started||mc.registration_thread!=GetCurrentThreadId()||mc.map)return 0;return mmc_register(&mc.registry,provider);}
static int mc_captured(void *walker){
 if(mc.thread!=GetCurrentThreadId()||walker!=dget(mc.walker)||!mc.visible)return 0;if(mc.input.drag)return 1;
 /* Cooperating Walker.Update observers can run before this frame's Steer.
  * Read the current pointer so a newly entered panel press cannot select a
  * stale native robot marker during that earlier observer. */
 MMCV2 pointer={0},local={0};int left=0,right=0,ok=1;if(!mc_pointer(&pointer,&left,&right))return 0;
 PTR canvas=dget(mc.canvas);DRect rect={0};PTR boxed=call(mc.rect,canvas,0,&ok);if(!boxed||!ok)return 0;dcopy(&rect,unbox(boxed),sizeof(rect));
 void *args[]={canvas,&pointer,0,&local};PTR converted=call(mc.local_point,0,args,&ok);
 return ok&&converted&&*(unsigned char*)unbox(converted)&&mmc_hit(mc.bounds,local.x-rect.x,rect.y+rect.h-local.y);
}
static int mc_context_active(void *walker){return mc.thread==GetCurrentThreadId()&&walker==dget(mc.walker)&&mc_context(walker,dget(mc.map),dget(mc.world));}
static const MoonMapControls mc_api={MMC_ABI,sizeof(MoonMapControls),mc_register_provider,mc_captured,mc_context_active};
__declspec(dllexport) const MoonMapControls *MoonLab_MapControls_v1(unsigned abi,unsigned size){return mc.started&&abi==MMC_ABI&&(size==24||size==sizeof(MoonMapControls))?&mc_api:0;}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
 (void)num;(void)dvector;(void)darray;
 if(mc.started)return 1;if(!bind_runtime(domain,game)||!dapi(game))return 0;mc.ground_menu_at=-1;mc.gate_frame=-1;mc.registration_thread=GetCurrentThreadId();mc.native_background=(DColor){.07f,.075f,.08f,.96f};mc.native_foreground=(DColor){.94f,.94f,.91f,1};mc.input.provider=mc.input.control=-1;mc.sector=-1;
 char path[1200];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",path,1000);if(n&&n<1000){cat(path+n,"\\map-control-framework.jsonl");mc.log=CreateFileA(path,0x40000000,7,0,2,0x80,0);if(mc.log==(PTR)(U64)-1)mc.log=0;}
#define MX(F,N,T) mc.F=(T)GetProcAddress(game,"il2cpp_" N);if(!mc.F){mc_event("missing export: " N);return 0;}
 MX(return_type,"method_get_return_type",PTR(*)(PTR));MX(field_type,"field_get_type",PTR(*)(PTR));MX(field_offset,"field_get_offset",int(*)(PTR));MX(field_set,"field_set_value",void(*)(PTR,PTR,void *));
#undef MX
#define MM(F,N,T,M,S) mc.F=dmethod(N,T,M,S);if(!mc.F){mc_event("missing: " T "::" M);return 0;}
 MM(alive,"UnityEngine","Object","op_Implicit","UnityEngine.Object");MM(restoring,"MoonCleaners","MoonWorld","get_IsRestoringRendering","");MM(focused,"UnityEngine","Application","get_isFocused","");MM(typing,"MoonCleaners","MoonInput","get_Typing","");MM(map_open,"MoonCleaners","MoonMapScreen","get_IsOpen","");
 MM(component,"UnityEngine","Component","GetComponent","System.Type");MM(pause_menu,"MoonCleaners","MoonWalker","get_PauseMenu","");MM(pause_blocks,"MoonCleaners","MoonPauseMenu","get_BlocksGameplay","");MM(pause_modal,"MoonCleaners","MoonPauseMenu","get_ModalOpen","");
 MM(get_transform,"UnityEngine","Component","get_transform","");MM(go_transform,"UnityEngine","GameObject","get_transform","");MM(go_ctor,"UnityEngine","GameObject",".ctor","System.String");MM(go_add,"UnityEngine","GameObject","AddComponent","System.Type");MM(go_component,"UnityEngine","GameObject","GetComponent","System.Type");MM(parent,"UnityEngine","Transform","SetParent","UnityEngine.Transform;System.Boolean");MM(destroy,"UnityEngine","Object","Destroy","UnityEngine.Object");MM(active,"UnityEngine","GameObject","SetActive","System.Boolean");
 MM(anchor_min,"UnityEngine","RectTransform","set_anchorMin","UnityEngine.Vector2");MM(anchor_max,"UnityEngine","RectTransform","set_anchorMax","UnityEngine.Vector2");MM(pivot,"UnityEngine","RectTransform","set_pivot","UnityEngine.Vector2");MM(anchored,"UnityEngine","RectTransform","set_anchoredPosition","UnityEngine.Vector2");MM(size,"UnityEngine","RectTransform","set_sizeDelta","UnityEngine.Vector2");MM(rect,"UnityEngine","RectTransform","get_rect","");
 MM(color,"UnityEngine.UI","Graphic","set_color","UnityEngine.Color");MM(raycast,"UnityEngine.UI","Graphic","set_raycastTarget","System.Boolean");MM(text,"TMPro","TMP_Text","set_text","System.String");MM(font,"TMPro","TMP_Text","set_font","TMPro.TMP_FontAsset");MM(font_size,"TMPro","TMP_Text","set_fontSize","System.Single");MM(text_color,"TMPro","TMP_Text","set_color","UnityEngine.Color");MM(text_align,"TMPro","TMP_Text","set_alignment","TMPro.TextAlignmentOptions");MM(text_wrap,"TMPro","TMP_Text","set_enableWordWrapping","System.Boolean");MM(text_overflow,"TMPro","TMP_Text","set_overflowMode","TMPro.TextOverflowModes");MM(text_rich,"TMPro","TMP_Text","set_richText","System.Boolean");MM(font_get,"MoonCleaners","MoonFonts","get_Text","");
 MM(frame,"UnityEngine","Time","get_frameCount","");MM(time,"UnityEngine","Time","get_unscaledTime","");MM(mouse,"UnityEngine.InputSystem","Mouse","get_current","");MM(mouse_position,"UnityEngine.InputSystem","Pointer","get_position","");MM(left_button,"UnityEngine.InputSystem","Mouse","get_leftButton","");MM(right_button,"UnityEngine.InputSystem","Mouse","get_rightButton","");MM(button_down,"UnityEngine.InputSystem.Controls","ButtonControl","get_isPressed","");MM(local_point,"UnityEngine","RectTransformUtility","ScreenPointToLocalPointInRectangle","UnityEngine.RectTransform;UnityEngine.Vector2;UnityEngine.Camera;UnityEngine.Vector2&");MM(pick,"MoonCleaners","MoonMapScreen","Pick","UnityEngine.Vector2");MM(moon_radius,"MoonCleaners","MoonMapScreen","MoonRadius","");
#undef MM
 const char *screen_names[]={"MoonUpgradeShop","MoonGarageMenu","MoonRobotMenu","MoonRadioMenu","MoonDepotBuilder","MoonHowToPlay"};
 for(unsigned i=0;i<6;i++){mc.screen_types[i]=dtype("MoonCleaners",screen_names[i]);mc.screen_open[i]=dmethod("MoonCleaners",screen_names[i],"get_IsOpen","");if(!mc.screen_types[i]||!mc.screen_open[i]){mc_event("native screen binding failed");return 0;}}
 PTR map=find_class("MoonCleaners","MoonMapScreen");mc.world_field=get_field(find_class("MoonCleaners","MoonWalker"),"world");
#define MF(F,N,T,O) mc.F=get_field(map,N);if(!mc.F||!mc_type(mc.field_type(mc.F),T)||mc.field_offset(mc.F)!=O){mc_event("field mismatch: " N);return 0;}
 MF(map_walker,"walker","MoonCleaners.MoonWalker",32);MF(map_pressing,"pressing","System.Boolean",216);MF(map_dragging,"dragging","System.Boolean",217);MF(map_over_panel,"overPanel","System.Boolean",219);MF(map_canvas,"canvasRect","UnityEngine.RectTransform",616);MF(map_sector,"hoveredSector","System.Int32",212);MF(map_mark,"hoveredMark","MoonCleaners.MoonMapScreen.Mark",264);MF(map_menu_open,"menuOpen","System.Boolean",424);MF(map_menu_ground,"menuOnGround","System.Boolean",425);MF(map_menu_spot,"menuSpot","UnityEngine.Vector2",436);MF(map_menu_at,"menuAt","System.Single",432);
#undef MF
 if(!mmc_ground_bind(&mc.ground,game)){mc_event("ground inverse binding failed");return 0;}
 if(!mc.world_field){mc_event("world field missing");return 0;}
 if(method_token(mc.pick)!=100668218){mc_event("pick token mismatch");return 0;}
 if(!mc_type(mc.return_type(mc.pick),"System.Nullable<UnityEngine.Vector3>")){mc_event("pick return mismatch");return 0;}
 before_hook=mc_before;
 if(!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game)){mc_event("walker hook prepare failed");return 0;}
 if(!prepare_hook(dmethod("MoonCleaners","MoonMapScreen","Steer","MoonCleaners.MoonWorld"),100668206,1,game)){mc_event("map hook prepare failed");return 0;}
 if(!install_hooks()){mc_event("hook registration failed");return 0;}
 if(moon_runtime_loader_present()){
  const MoonRuntimeApiV1 *runtime=moon_runtime_connect();const MoonServiceLimitV1 limits[]={{"max_providers",MMC_PROVIDER_LIMIT},{"max_units",MMC_UNIT_LIMIT},{"max_actions",MMC_ACTION_LIMIT}};
  MoonServicePublicationLimitsV1 publication={.base={.size=sizeof(publication),.service_id="map-controls",.abi=MMC_ABI,.api_size=sizeof(mc_api),.api=&mc_api},.limit_count=3,.reserved=0,.limits=limits};
  if(!runtime||!runtime->publish(&publication.base)){mc_event("service publication failed");return 0;}
 }
 mc.started=1;mc_event("ready");return 1;
}
