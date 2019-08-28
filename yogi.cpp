/** libs **/
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

/** defines **/

#define CTRL_KEY(k) ((k) & 0x1f)
using namespace std;


/** data **/
struct editorConfig
{	
	int screenrows;
	int screencols;
	struct termios orig_termios;	
};
struct editorConfig E;

/**Terminal **/
void die(const char *s)
{

	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);	
	perror(s);
	exit(1);
}

void disableRawMode()
{
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH , &E.orig_termios) == -1)
		die("tcsetattr");
}

void enableRawMode()
{
	if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die ("tcsetattr");
	atexit(disableRawMode);
	struct termios raw = E.orig_termios;
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | ISIG );
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
	if (tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) == -1) die("tcsetattr");
}

char editorReadKey()
{
	int nread;
	char c;
	while((nread = read(STDIN_FILENO, &c , 1))!=1)
	{
		if (nread == -1 && errno != EAGAIN ) die ("read");
	}
	return c;
}

int getWindowSize(int *rows, int *cols)
{
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)==-1 || ws.ws_col ==0)
	{
		return -1;
	}
	else
	{
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}



/** Append Buffer **/

struct abuf
{
	char *b;
	int len;
};

#define ABUF_INIT {NULL,0}

void abAppend(struct abuf *ab, const char *s, int len)
{
	char *news = realloc(ab->b, ab->len + len);

	if (news == NULL) return;
	memcpy(&news[ab->len],s,len);
	ab->b = news;
	ab->len+=len;
}

void abFree(struct abuf *ab)
{
	free(ab->b);
}

/** Output **/


void editorDrawRows(struct abuf *ab)
{
	int i;
	for (i = 0; i <E.screenrows ; ++i)
	{
		abAppend(ab,"~",1);
		if (i < E.screenrows - 1)
		{
			abAppend(ab, "\r\n", 2);
		}
	}
}

void editorRefreshScreen()
{
	struct abuf ab = ABUF_INIT;

	abAppend(&ab, "\x1b[2J", 4);
	abAppend(&ab, "\x1b[H", 3);

	editorDrawRows(&ab);
	abAppend(&ab, "\x1b[H", 3);
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

/** Input **/

void editorProcessKeypress()
{
	char c = editorReadKey();
	switch(c)
	{
		case CTRL_KEY('q'):
		write(STDOUT_FILENO, "\x1b[2J", 4);
		write(STDOUT_FILENO, "\x1b[H", 3);
		exit(0);
		break;
	}
}

/** Initialisation **/

void initEditor()
{
	if (getWindowSize(&E.screenrows, &E.screencols)==-1) die("getWindowSize");
	{
		/* code */
	}
}

int main()
{
	enableRawMode();
	initEditor();
	while (1)
	{
		editorRefreshScreen();
		editorProcessKeypress();
	}
	return 0;
}