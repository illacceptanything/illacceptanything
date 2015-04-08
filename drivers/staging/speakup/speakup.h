#ifndef _SPEAKUP_H
#define _SPEAKUP_H
#include <linux/version.h>

#include "spk_types.h"
#include "i18n.h"

#define SPEAKUP_VERSION "3.1.6"
#define KEY_MAP_VER 119
#define SHIFT_TBL_SIZE 64
#define MAX_DESC_LEN 72

/* proc permissions */
#define USER_R (S_IFREG|S_IRUGO)
#define USER_W (S_IFREG|S_IWUGO)
#define USER_RW (S_IFREG|S_IRUGO|S_IWUGO)
#define ROOT_W (S_IFREG|S_IRUGO|S_IWUSR)

#define TOGGLE_0 .u.n = {NULL, 0, 0, 1, 0, 0, NULL }
#define TOGGLE_1 .u.n = {NULL, 1, 0, 1, 0, 0, NULL }
#define MAXVARLEN 15

#define SYNTH_OK 0x0001
#define B_ALPHA 0x0002
#define ALPHA 0x0003
#define B_CAP 0x0004
#define A_CAP 0x0007
#define B_NUM 0x0008
#define NUM 0x0009
#define ALPHANUM (B_ALPHA|B_NUM)
#define SOME 0x0010
#define MOST 0x0020
#define PUNC 0x0040
#define A_PUNC 0x0041
#define B_WDLM 0x0080
#define WDLM 0x0081
#define B_EXNUM 0x0100
#define CH_RPT 0x0200
#define B_CTL 0x0400
#define A_CTL (B_CTL+SYNTH_OK)
#define B_SYM 0x0800
#define B_CAPSYM (B_CAP|B_SYM)

#define IS_WDLM(x) (spk_chartab[((u_char)x)]&B_WDLM)
#define IS_CHAR(x, type) (spk_chartab[((u_char)x)]&type)
#define IS_TYPE(x, type) ((spk_chartab[((u_char)x)]&type) == type)

#define SET_DEFAULT -4
#define E_RANGE -3
#define E_TOOLONG -2
#define E_UNDEF -1

extern int speakup_thread(void *data);
extern void reset_default_chars(void);
extern void reset_default_chartab(void);
extern void synth_start(void);
void synth_insert_next_index(int sent_num);
void reset_index_count(int sc);
void get_index_count(int *linecount, int *sentcount);
extern int set_key_info(const u_char *key_info, u_char *k_buffer);
extern char *strlwr(char *s);
extern char *speakup_s2i(char *start, int *dest);
extern char *s2uchar(char *start, char *dest);
extern char *xlate(char *s);
extern int speakup_kobj_init(void);
extern void speakup_kobj_exit(void);
extern int chartab_get_value(char *keyword);
extern void speakup_register_var(struct var_t *var);
extern void speakup_unregister_var(enum var_id_t var_id);
extern struct st_var_header *get_var_header(enum var_id_t var_id);
extern struct st_var_header *var_header_by_name(const char *name);
extern struct punc_var_t *get_punc_var(enum var_id_t var_id);
extern int set_num_var(int val, struct st_var_header *var, int how);
extern int set_string_var(const char *page, struct st_var_header *var, int len);
extern int set_mask_bits(const char *input, const int which, const int how);
extern special_func special_handler;
extern int handle_help(struct vc_data *vc, u_char type, u_char ch, u_short key);
extern int synth_init(char *name);
extern void synth_release(void);

extern void do_flush(void);
extern void speakup_start_ttys(void);
extern void synth_buffer_add(char ch);
extern void synth_buffer_clear(void);
extern void speakup_clear_selection(void);
extern int speakup_set_selection(struct tty_struct *tty);
extern int speakup_paste_selection(struct tty_struct *tty);
extern void speakup_register_devsynth(void);
extern void speakup_unregister_devsynth(void);
extern void synth_write(const char *buf, size_t count);
extern int synth_supports_indexing(void);

extern struct vc_data *spk_sel_cons;
extern unsigned short xs, ys, xe, ye; /* our region points */

extern wait_queue_head_t speakup_event;
extern struct kobject *speakup_kobj;
extern struct task_struct *speakup_task;
extern const u_char key_defaults[];

/* Protect speakup synthesizer list */
extern struct mutex spk_mutex;
extern struct st_spk_t *speakup_console[];
extern struct spk_synth *synth;
extern char pitch_buff[];
extern u_char *our_keys[];
extern short punc_masks[];
extern char str_caps_start[], str_caps_stop[];
extern const struct st_bits_data punc_info[];
extern u_char key_buf[600];
extern char *characters[];
extern char *default_chars[];
extern u_short spk_chartab[];
extern int no_intr, say_ctrl, say_word_ctl, punc_level;
extern int reading_punc, attrib_bleep, bleeps;
extern int bleep_time, bell_pos;
extern int spell_delay, key_echo;
extern short punc_mask;
extern short pitch_shift, synth_flags;
extern int quiet_boot;
extern char *synth_name;
extern struct bleep unprocessed_sound;

/* Prototypes from fakekey.c. */
int speakup_add_virtual_keyboard(void);
void speakup_remove_virtual_keyboard(void);
void speakup_fake_down_arrow(void);
bool speakup_fake_key_pressed(void);

#endif
