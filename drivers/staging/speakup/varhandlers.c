#include <linux/ctype.h>
#include "spk_types.h"
#include "spk_priv.h"
#include "speakup.h"

static struct st_var_header var_headers[] = {
	{ "version", VERSION, VAR_PROC, NULL, NULL },
	{ "synth_name", SYNTH, VAR_PROC, NULL, NULL },
	{ "keymap", KEYMAP, VAR_PROC, NULL, NULL },
	{ "silent", SILENT, VAR_PROC, NULL, NULL },
	{ "punc_some", PUNC_SOME, VAR_PROC, NULL, NULL },
	{ "punc_most", PUNC_MOST, VAR_PROC, NULL, NULL },
	{ "punc_all", PUNC_ALL, VAR_PROC, NULL, NULL },
	{ "delimiters", DELIM, VAR_PROC, NULL, NULL },
	{ "repeats", REPEATS, VAR_PROC, NULL, NULL },
	{ "ex_num", EXNUMBER, VAR_PROC, NULL, NULL },
	{ "characters", CHARS, VAR_PROC, NULL, NULL },
	{ "synth_direct", SYNTH_DIRECT, VAR_PROC, NULL, NULL },
	{ "caps_start", CAPS_START, VAR_STRING, str_caps_start, NULL },
	{ "caps_stop", CAPS_STOP, VAR_STRING, str_caps_stop, NULL },
	{ "delay_time", DELAY, VAR_TIME, NULL, NULL },
	{ "trigger_time", TRIGGER, VAR_TIME, NULL, NULL },
	{ "jiffy_delta", JIFFY, VAR_TIME, NULL, NULL },
	{ "full_time", FULL, VAR_TIME, NULL, NULL },
	{ "spell_delay", SPELL_DELAY, VAR_NUM, &spell_delay, NULL },
	{ "bleeps", BLEEPS, VAR_NUM, &bleeps, NULL },
	{ "attrib_bleep", ATTRIB_BLEEP, VAR_NUM, &attrib_bleep, NULL },
	{ "bleep_time", BLEEP_TIME, VAR_TIME, &bleep_time, NULL },
	{ "cursor_time", CURSOR_TIME, VAR_TIME, NULL, NULL },
	{ "punc_level", PUNC_LEVEL, VAR_NUM, &punc_level, NULL },
	{ "reading_punc", READING_PUNC, VAR_NUM, &reading_punc, NULL },
	{ "say_control", SAY_CONTROL, VAR_NUM, &say_ctrl, NULL },
	{ "say_word_ctl", SAY_WORD_CTL, VAR_NUM, &say_word_ctl, NULL },
	{ "no_interrupt", NO_INTERRUPT, VAR_NUM, &no_intr, NULL },
	{ "key_echo", KEY_ECHO, VAR_NUM, &key_echo, NULL },
	{ "bell_pos", BELL_POS, VAR_NUM, &bell_pos, NULL },
	{ "rate", RATE, VAR_NUM, NULL, NULL },
	{ "pitch", PITCH, VAR_NUM, NULL, NULL },
	{ "vol", VOL, VAR_NUM, NULL, NULL },
	{ "tone", TONE, VAR_NUM, NULL, NULL },
	{ "punct", PUNCT, VAR_NUM, NULL, NULL   },
	{ "voice", VOICE, VAR_NUM, NULL, NULL },
	{ "freq", FREQUENCY, VAR_NUM, NULL, NULL },
	{ "lang", LANG, VAR_NUM, NULL, NULL },
	{ "chartab", CHARTAB, VAR_PROC, NULL, NULL },
	{ "direct", DIRECT, VAR_NUM, NULL, NULL },
};

static struct st_var_header *var_ptrs[MAXVARS] = { 0, 0, 0 };

static struct punc_var_t punc_vars[] = {
	{ PUNC_SOME, 1 },
	{ PUNC_MOST, 2 },
	{ PUNC_ALL, 3 },
	{ DELIM, 4 },
	{ REPEATS, 5 },
	{ EXNUMBER, 6 },
	{ -1, -1 },
};

int chartab_get_value(char *keyword)
{
	int value = 0;

	if (!strcmp(keyword, "ALPHA"))
		value = ALPHA;
	else if (!strcmp(keyword, "B_CTL"))
		value = B_CTL;
	else if (!strcmp(keyword, "WDLM"))
		value = WDLM;
	else if (!strcmp(keyword, "A_PUNC"))
		value = A_PUNC;
	else if (!strcmp(keyword, "PUNC"))
		value = PUNC;
	else if (!strcmp(keyword, "NUM"))
		value = NUM;
	else if (!strcmp(keyword, "A_CAP"))
		value = A_CAP;
	else if (!strcmp(keyword, "B_CAPSYM"))
		value = B_CAPSYM;
	else if (!strcmp(keyword, "B_SYM"))
		value = B_SYM;
	return value;
}

void speakup_register_var(struct var_t *var)
{
	static char nothing[2] = "\0";
	int i;
	struct st_var_header *p_header;

	BUG_ON(!var || var->var_id < 0 || var->var_id >= MAXVARS);
	if (var_ptrs[0] == NULL) {
		for (i = 0; i < MAXVARS; i++) {
			p_header = &var_headers[i];
			var_ptrs[p_header->var_id] = p_header;
			p_header->data = NULL;
		}
	}
	p_header = var_ptrs[var->var_id];
	if (p_header->data != NULL)
		return;
	p_header->data = var;
	switch (p_header->var_type) {
	case VAR_STRING:
		set_string_var(nothing, p_header, 0);
		break;
	case VAR_NUM:
	case VAR_TIME:
		set_num_var(0, p_header, E_DEFAULT);
		break;
	default:
		break;
	}
	return;
}

void speakup_unregister_var(enum var_id_t var_id)
{
	struct st_var_header *p_header;
	BUG_ON(var_id < 0 || var_id >= MAXVARS);
	p_header = var_ptrs[var_id];
	p_header->data = NULL;
}

struct st_var_header *get_var_header(enum var_id_t var_id)
{
	struct st_var_header *p_header;
	if (var_id < 0 || var_id >= MAXVARS)
		return NULL;
	p_header = var_ptrs[var_id];
	if (p_header->data == NULL)
		return NULL;
	return p_header;
}

struct st_var_header *var_header_by_name(const char *name)
{
	int i;
	struct st_var_header *where = NULL;

	if (name != NULL) {
		i = 0;
		while ((i < MAXVARS) && (where == NULL)) {
			if (strcmp(name, var_ptrs[i]->name) == 0)
				where = var_ptrs[i];
			else
				i++;
		}
	}
	return where;
}

struct var_t *get_var(enum var_id_t var_id)
{
	BUG_ON(var_id < 0 || var_id >= MAXVARS);
	BUG_ON(!var_ptrs[var_id]);
	return var_ptrs[var_id]->data;
}
EXPORT_SYMBOL_GPL(get_var);

struct punc_var_t *get_punc_var(enum var_id_t var_id)
{
	struct punc_var_t *rv = NULL;
	struct punc_var_t *where;

	where = punc_vars;
	while ((where->var_id != -1) && (rv == NULL)) {
		if (where->var_id == var_id)
			rv = where;
		else
			where++;
	}
	return rv;
}

/* handlers for setting vars */
int set_num_var(int input, struct st_var_header *var, int how)
{
	int val;
	short ret = 0;
	int *p_val = var->p_val;
	int l;
	char buf[32];
	char *cp;
	struct var_t *var_data = var->data;
	if (var_data == NULL)
		return E_UNDEF;

	if (how == E_NEW_DEFAULT) {
		if (input < var_data->u.n.low || input > var_data->u.n.high)
			ret = E_RANGE;
		else
			var_data->u.n.default_val = input;
		return ret;
	}
	if (how == E_DEFAULT) {
		val = var_data->u.n.default_val;
		ret = SET_DEFAULT;
	} else {
		if (how == E_SET)
			val = input;
		else
			val = var_data->u.n.value;
		if (how == E_INC)
			val += input;
		else if (how == E_DEC)
			val -= input;
		if (val < var_data->u.n.low || val > var_data->u.n.high)
			return E_RANGE;
	}
	var_data->u.n.value = val;
	if (var->var_type == VAR_TIME && p_val != NULL) {
		*p_val = msecs_to_jiffies(val);
		return ret;
	}
	if (p_val != NULL)
		*p_val = val;
	if (var->var_id == PUNC_LEVEL) {
		punc_mask = punc_masks[val];
		return ret;
	}
	if (var_data->u.n.multiplier != 0)
		val *= var_data->u.n.multiplier;
	val += var_data->u.n.offset;
	if (var->var_id < FIRST_SYNTH_VAR || synth == NULL)
		return ret;
	if (synth->synth_adjust != NULL) {
		int status = synth->synth_adjust(var);
		return (status != 0) ? status : ret;
	}
	if (!var_data->u.n.synth_fmt)
		return ret;
	if (var->var_id == PITCH)
		cp = pitch_buff;
	else
		cp = buf;
	if (!var_data->u.n.out_str)
		l = sprintf(cp, var_data->u.n.synth_fmt, (int)val);
	else
		l = sprintf(cp,
			var_data->u.n.synth_fmt, var_data->u.n.out_str[val]);
	synth_printf("%s", cp);
	return ret;
}

int set_string_var(const char *page, struct st_var_header *var, int len)
{
	int ret = 0;
	struct var_t *var_data = var->data;
	if (var_data == NULL)
		return E_UNDEF;
	if (len > MAXVARLEN)
		return -E_TOOLONG;
	if (!len) {
		if (!var_data->u.s.default_val)
			return 0;
		ret = SET_DEFAULT;
		if (!var->p_val)
			var->p_val = var_data->u.s.default_val;
		if (var->p_val != var_data->u.s.default_val)
			strcpy((char *)var->p_val, var_data->u.s.default_val);
	} else if (var->p_val)
		strcpy((char *)var->p_val, page);
	else
		return -E_TOOLONG;
	return ret;
}

/* set_mask_bits sets or clears the punc/delim/repeat bits,
 * if input is null uses the defaults.
 * values for how: 0 clears bits of chars supplied,
 * 1 clears allk, 2 sets bits for chars */
int set_mask_bits(const char *input, const int which, const int how)
{
	u_char *cp;
	short mask = punc_info[which].mask;
	if (how&1) {
		for (cp = (u_char *)punc_info[3].value; *cp; cp++)
			spk_chartab[*cp] &= ~mask;
	}
	cp = (u_char *)input;
	if (cp == 0)
		cp = punc_info[which].value;
	else {
		for ( ; *cp; cp++) {
			if (*cp < SPACE)
				break;
			if (mask < PUNC) {
				if (!(spk_chartab[*cp]&PUNC))
					break;
			} else if (spk_chartab[*cp]&B_NUM)
				break;
		}
		if (*cp)
			return -EINVAL;
		cp = (u_char *)input;
	}
	if (how&2) {
		for ( ; *cp; cp++)
			if (*cp > SPACE)
				spk_chartab[*cp] |= mask;
	} else {
		for ( ; *cp; cp++)
			if (*cp > SPACE)
				spk_chartab[*cp] &= ~mask;
	}
	return 0;
}

char *strlwr(char *s)
{
	char *p;
	if (s == NULL)
		return NULL;

	for (p = s; *p; p++)
		*p = tolower(*p);
	return s;
}

char *speakup_s2i(char *start, int *dest)
{
	int val;
	char ch = *start;
	if (ch == '-' || ch == '+')
		start++;
	if (*start < '0' || *start > '9')
		return start;
	val = (*start) - '0';
	start++;
	while (*start >= '0' && *start <= '9') {
		val *= 10;
		val += (*start) - '0';
		start++;
	}
	if (ch == '-')
		*dest = -val;
	else
		*dest = val;
	return start;
}

char *s2uchar(char *start, char *dest)
{
	int val = 0;
	while (*start && *start <= SPACE)
		start++;
	while (*start >= '0' && *start <= '9') {
		val *= 10;
		val += (*start) - '0';
		start++;
	}
	if (*start == ',')
		start++;
	*dest = (u_char)val;
	return start;
}

char *xlate(char *s)
{
	static const char finds[] = "nrtvafe";
	static const char subs[] = "\n\r\t\013\001\014\033";
	static const char hx[] = "0123456789abcdefABCDEF";
	char *p = s, *p1, *p2, c;
	int num;
	while ((p = strchr(p, '\\'))) {
		p1 = p+1;
		p2 = strchr(finds, *p1);
		if (p2) {
			*p++ = subs[p2-finds];
			p1++;
		} else if (*p1 >= '0' && *p1 <= '7') {
			num = (*p1++)&7;
			while (num < 256 && *p1 >= '0' && *p1 <= '7') {
				num <<= 3;
				num = (*p1++)&7;
			}
			*p++ = num;
		} else if (*p1 == 'x' &&
				strchr(hx, p1[1]) && strchr(hx, p1[2])) {
			p1++;
			c = *p1++;
			if (c > '9')
				c = (c - '7') & 0x0f;
			else
				c -= '0';
			num = c << 4;
			c = *p1++;
			if (c > '9')
				c = (c-'7')&0x0f;
			else
				c -= '0';
			num += c;
			*p++ = num;
		} else
			*p++ = *p1++;
		p2 = p;
		while (*p1)
			*p2++ = *p1++;
		*p2 = '\0';
	}
	return s;
}
