#ifndef SIMPLE_H
#define SIMPLE_H
 
void simple_init(Window* window);
void simple_deinit(void);
void simple_set_style(bool inverse);
void simple_update_time(struct tm *tick_time);
void simple_update_bounds(void);

#endif /* SIMPLE_H */
