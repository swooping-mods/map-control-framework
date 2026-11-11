/* Map.Update owns walker.uiBlocked too; use explicit other-screen admission. */
static int mmc_screen_input_allowed(int map_open,int focused,int typing,int restoring,int pause_blocks,int modal_open,unsigned other_screens){
 return map_open&&focused&&!typing&&!restoring&&!pause_blocks&&!modal_open&&!other_screens;
}
