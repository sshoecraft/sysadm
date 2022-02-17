#ifdef __MINGW32__
#include <stdio.h>
/* For PASS_MAX. */
#include <limits.h>
/* For _getch(). */
#include <conio.h>
/* For strdup(). */
#include <string.h>

#ifndef PASS_MAX
# define PASS_MAX 512
#endif

char *
getpass (const char *prompt)
{
  char getpassbuf[PASS_MAX + 1];
  size_t i = 0;
  int c;

  if (prompt)
    {
      fputs (prompt, stderr);
      fflush (stderr);
    }

  for (;;)
    {
      c = _getch ();
      if (c == '\r')
	{
	  getpassbuf[i] = '\0';
	  break;
	}
      else if (i < PASS_MAX)
	{
	  getpassbuf[i++] = c;
	}

      if (i >= PASS_MAX)
	{
	  getpassbuf[i] = '\0';
	  break;
	}
    }

  if (prompt)
    {
      fputs ("\r\n", stderr);
      fflush (stderr);
    }

  return strdup (getpassbuf);
}
#endif
