#include <stdio.h>
     #include <setjmp.h>
     #include <signal.h>
     #include <unistd.h>
     jmp_buf env; static void signal_handler();

     main()  {
             int returned_from_longjump, processing = 1;
             unsigned int time_interval = 4;
		int val;
		    sigset_t sigset;
             if ((returned_from_longjump = setjmp(env)) != 0)
                 switch (returned_from_longjump)     {
                   case SIGINT:
                     printf("longjumped from interrupt %d\n",SIGINT);
                     break;
                   case SIGALRM:
                     printf("longjumped from alarm %d\n",SIGALRM);
    sigfillset(&sigset);
    sigprocmask(SIG_UNBLOCK,&sigset,NULL);
                     break;
                 }
     //        (void) signal(SIGINT, signal_handler);
             (void) signal(SIGALRM, signal_handler);
             val = alarm(time_interval);
		printf("val: %d\n", val);
             while (processing)        {
               printf(" waiting for you to INTERRUPT (cntrl-C) ...\n");
               sleep(1);
             }       /* end while forever loop */
     }

     static void signal_handler(sig)
     int sig; {
             switch (sig)     {
               case SIGINT:   /* process for interrupt */
                              longjmp(env,sig);
                                     /* break never reached */
               case SIGALRM:  /* process for alarm */
                              longjmp(env,sig);
                                    /* break never reached */
               default:       exit(sig);
             }
     }
