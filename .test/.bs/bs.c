/*
 *  bs.c  –  fs bo timer with pj
 *
 *  gcc -O2 bs.c -o bs -lX11 -lXtst
 *
 *  use: ./bs <time> <s|m|h>
 *  exit: q
 */

#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>

static double
parse_seconds(const char *arg, char unit)
{
    double v = atof(arg);
    switch (unit) {
        case 's': return v;
        case 'm': return v * 60.0;
        case 'h': return v * 3600.0;
        default:
            fprintf(stderr, "unit must be s, m, or h\n");
            exit(1);
    }
}

/* millisecond sleep */
static void msleep(long ms)
{
    struct timespec ts = {
        .tv_sec  = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000L
    };
    nanosleep(&ts, NULL);
}
static void
fake_key(Display *dpy, KeySym ks, int press)
{
    KeyCode code = XKeysymToKeycode(dpy, ks);
    if (code) XTestFakeKeyEvent(dpy, code, press, CurrentTime);
}

static void
kp_logout(Display *dpy)
{
    /* Ctrl‑Alt‑Delete, →, Enter */
    fake_key(dpy, XK_Control_L, True);
    fake_key(dpy, XK_Alt_L, True);
    fake_key(dpy, XK_Delete, True);
    fake_key(dpy, XK_Delete, False);
    fake_key(dpy, XK_Alt_L, False);
    fake_key(dpy, XK_Control_L, False);
    XFlush(dpy);
    msleep(100);

    fake_key(dpy, XK_Right, True);
    fake_key(dpy, XK_Right, False);
    XFlush(dpy);
    msleep(50);

    fake_key(dpy, XK_Return, True);
    fake_key(dpy, XK_Return, False);
    XFlush(dpy);
}

static void logout(Display *dpy)
{
    if (system("gnome-session-quit --logout --no-prompt") != 0)
        kp_logout(dpy);
}

int
main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <time> <s|m|h>\n", argv[0]);
        return 1;
    }

    const double total = parse_seconds(argv[1], argv[2][0]);

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) { perror("XOpenDisplay"); return 1; }

    const int scr   = DefaultScreen(dpy);
    const int w     = DisplayWidth(dpy, scr);
    const int h     = DisplayHeight(dpy, scr);
    Window root     = RootWindow(dpy, scr);

    /* full‑screen black window, bypassing the WM */
    XSetWindowAttributes a = { .override_redirect = True,
                               .background_pixel  = BlackPixel(dpy, scr) };
    Window win = XCreateWindow(dpy, root, 0, 0, w, h, 0,
                               CopyFromParent, InputOutput, CopyFromParent,
                               CWOverrideRedirect | CWBackPixel, &a);

    XSelectInput(dpy, win,
             KeyPressMask | ButtonPressMask | PointerMotionMask);
    XMapRaised(dpy, win);
    /* ensure we receive key events (for 'q') */
    XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
    XFlush(dpy);

    /* park pointer bottom‑right */
    XWarpPointer(dpy, None, root, 0, 0, 0, 0, w - 1, h - 1);
    XFlush(dpy);

    int fd = ConnectionNumber(dpy);
    struct timeval tv;
    time_t start = time(NULL);
    int x = w - 1, y = h - 1;

    while (difftime(time(NULL), start) < total) {
        tv.tv_sec  = 1;
        tv.tv_usec = 0;
        fd_set r; FD_ZERO(&r); FD_SET(fd, &r);

        int n = select(fd + 1, &r, NULL, NULL, &tv);

        if (n > 0 && FD_ISSET(fd, &r)) {
            XEvent ev;
            while (XPending(dpy)) {
                XNextEvent(dpy, &ev);


                if (ev.type == KeyPress) {
                    KeySym sym = XLookupKeysym(&ev.xkey, 0);
                    if (sym == XK_q || sym == XK_Q) {
                        /* quit without logging out */
                        XDestroyWindow(dpy, win);
                        XCloseDisplay(dpy);
                        return 0;
                    } else {
                        logout(dpy);
                        return 0;
                    }
                }

                if (ev.type == ButtonPress) {
                    logout(dpy);
                    return 0;
                }

                if (ev.type == MotionNotify) {
                    XMotionEvent *m = &ev.xmotion;
                    /* user moved if coords differ from our last warp target */
                    if (abs(m->x_root - x) > 333 || abs(m->y_root - y) > 222) {
                        logout(dpy);
                        return 0;
                    }
                }
            }
        }
        else
        {
            /* timer tick – jiggle pointer 1 px left, wrap at 0 */
            if (--x < 0) x = w - 1;
            XWarpPointer(dpy, None, root, 0, 0, 0, 0, x, y);
            XFlush(dpy);
        }
    }
    logout(dpy);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    return 0;
}
