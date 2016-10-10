#ifndef SIMPLEBIG_H
#define SIMPLEBIG_H
 
void simplebig_init(Window* window);
void simplebig_deinit(void);
void simplebig_set_style(bool inverse);
void simplebig_update_time(struct tm *tick_time);
void simplebig_update_bounds(void);

#endif /* SIMPLEBIG_H */
