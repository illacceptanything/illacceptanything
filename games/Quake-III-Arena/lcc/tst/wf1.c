/* wf1 - print word frequencies; uses structures */

struct node {
	int count;		/* frequency count */
	struct node *left;	/* left subtree */
	struct node *right;	/* right subtree */
	char *word;		/* word itself */
} words[2000];
int next;		/* index of next free entry in words */

struct node *lookup();

main() {
	struct node *root;
	char word[20];

	root = 0;
	next = 0;
	while (getword(word))
		lookup(word, &root)->count++;
	tprint(root);
	return 0;
}

/* err - print error message s and die	*/
err(s) char *s; {
	printf("? %s\n", s);
	exit(1);
}

/* getword - get next input word into buf, return 0 on EOF */
int getword(buf) char *buf; {
	char *s;
	int c;

	while ((c = getchar()) != -1 && isletter(c) == 0)
		;
	for (s = buf; c = isletter(c); c = getchar())
		*s++ = c;
	*s = 0;
	if (s > buf)
		return (1);
	return (0);
}

/* isletter - return folded version of c if it is a letter, 0 otherwise */
int isletter(c) {
	if (c >= 'A' && c <= 'Z')
		c += 'a' - 'A';
	if (c >= 'a' && c <= 'z')
		return (c);
	return (0);
}

/* lookup - lookup word in tree; install if necessary */
struct node *lookup(word, p) char *word; struct node **p; {
	int cond;
	char *malloc();

	if (*p) {
		cond = strcmp(word, (*p)->word);
		if (cond < 0)
			return lookup(word, &(*p)->left);
		else if (cond > 0)
			return lookup(word, &(*p)->right);
		else
			return *p;
	}
	if (next >= 2000)
		err("out of node storage");
	words[next].count = 0;
	words[next].left = words[next].right = 0;
	words[next].word = malloc(strlen(word) + 1);
	if (words[next].word == 0)
		err("out of word storage");
	strcpy(words[next].word, word);
	return *p = &words[next++];
}

/* tprint - print tree */
tprint(tree) struct node *tree; {
	if (tree) {
		tprint(tree->left);
		printf("%d\t%s\n", tree->count, tree->word);
		tprint(tree->right);
	}
}

/* strcmp - compare s1 and s2, return <0, 0, or >0 */
int strcmp(s1, s2) char *s1, *s2; {
	while (*s1 == *s2) {
		if (*s1++ == 0)
			return 0;
		++s2;
	}
	if (*s1 == 0)
		return -1;
	else if (*s2 == 0)
		return 1;
	return *s1 - *s2;
}
