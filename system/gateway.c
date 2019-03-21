#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

/* numeric: & 0x8000: default instead of *,
 *          & 0x4000: host instead of net,
 *          & 0x0fff: don't resolve
 */
char *INET_rresolve(struct sockaddr_in *s_in, int numeric, uint32_t netmask)
{
    /* addr-to-name cache */
    struct addr
    {
        struct addr *next;
        struct sockaddr_in addr;
        int host;
        char name[1];
    };
    /*
      static struct addr *cache = NULL;

      struct addr *pn;
      char *name;
      uint32_t ad, host_ad;
      int host = 0;
    */
    uint32_t ad;

    /* Grmpf. -FvK */
    if(s_in->sin_family != AF_INET)
    {
#ifdef DEBUG
        printf("rresolve: unsupported address family %d!",
               s_in->sin_family);
#endif
        errno = EAFNOSUPPORT;
        return NULL;
    }
    ad = s_in->sin_addr.s_addr;
#ifdef DEBUG
    printf("rresolve: %08x, mask %08x, num %08x", (unsigned)ad, netmask, numeric);
#endif
    if(ad == INADDR_ANY)
    {
        if((numeric & 0x0FFF) == 0)
        {
            if(numeric & 0x8000)
                return strdup("default");
            return strdup("*");
        }
    }
    if(numeric & 0x0FFF)
        return strdup(inet_ntoa(s_in->sin_addr));

    return 0;
}

#ifndef RTF_UP
/* Keep this in sync with /usr/src/linux/include/linux/route.h */
#define RTF_UP          0x0001  /* route usable                 */
#define RTF_GATEWAY     0x0002  /* destination is a gateway     */
#define RTF_HOST        0x0004  /* host entry (net otherwise)   */
#define RTF_REINSTATE   0x0008  /* reinstate route after tmout  */
#define RTF_DYNAMIC     0x0010  /* created dyn. (by redirect)   */
#define RTF_MODIFIED    0x0020  /* modified dyn. (by redirect)  */
#define RTF_MTU         0x0040  /* specific MTU for this route  */
#ifndef RTF_MSS
#define RTF_MSS         RTF_MTU /* Compatibility :-(            */
#endif
#define RTF_WINDOW      0x0080  /* per route window clamping    */
#define RTF_IRTT        0x0100  /* Initial round trip time      */
#define RTF_REJECT      0x0200  /* Reject route                 */
#endif

static const unsigned flagvals[] =   /* Must agree with flagchars[]. */
{
    RTF_GATEWAY,
    RTF_HOST,
    RTF_REINSTATE,
    RTF_DYNAMIC,
    RTF_MODIFIED,
#if ENABLE_FEATURE_IPV6
    RTF_DEFAULT,
    RTF_ADDRCONF,
    RTF_CACHE
#endif
};

#define IPV4_MASK (RTF_GATEWAY|RTF_HOST|RTF_REINSTATE|RTF_DYNAMIC|RTF_MODIFIED)
#define IPV6_MASK (RTF_GATEWAY|RTF_HOST|RTF_DEFAULT|RTF_ADDRCONF|RTF_CACHE)

/* Must agree with flagvals[]. */
static const char flagchars[] =
    "GHRDM"
#if ENABLE_FEATURE_IPV6
    "DAC"
#endif
    ;

static void set_flags(char *flagstr, int flags)
{
    int i;

    *flagstr++ = 'U';

    for(i = 0; (*flagstr = flagchars[i]) != 0; i++)
    {
        if(flags & flagvals[i])
        {
            ++flagstr;
        }
    }
}

const char *SxNetDefaultGateway(char *ifname)
{
    char devname[64], flags[16];
    static char *sdest = 0, *sgw = 0;
    unsigned long d, g, m;
    int flgs, ref, use, metric, mtu, win, ir;
    struct sockaddr_in s_addr;
    struct in_addr mask;

    FILE *fp = fopen("/proc/net/route", "r");
    if(!fp)
        return 0;

    if(fscanf(fp, "%*[^\n]\n") < 0)    /* Skip the first line. */
    {
        goto ERROR;      /* Empty or missing line, or read error. */
    }
    while(1)
    {
        int r;
        r = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
                   devname, &d, &g, &flgs, &ref, &use, &metric, &m,
                   &mtu, &win, &ir);
        if(r != 11)
        {
            if((r < 0) && feof(fp))    /* EOF with no (nonspace) chars read. */
            {
                break;
            }
ERROR:
            fclose(fp);
            perror("fscanf");
            return 0;
        }

        /* if device name is same, check next. */
        if(strcmp(devname, ifname) != 0)
            continue;

        if(!(flgs & RTF_UP))    /* Skip interfaces that are down. */
        {
            continue;
        }

        set_flags(flags, (flgs & IPV4_MASK));
#ifdef RTF_REJECT
        if(flgs & RTF_REJECT)
        {
            flags[0] = '!';
        }
#endif
        if(sdest) free(sdest);
        if(sgw)  free(sgw);

        memset(&s_addr, 0, sizeof(struct sockaddr_in));
        s_addr.sin_family = AF_INET;
        s_addr.sin_addr.s_addr = d;
        sdest = INET_rresolve(&s_addr, (0x0fff | 0x8000), m); /* 'default' instead of '*' */
        s_addr.sin_addr.s_addr = g;
        sgw = INET_rresolve(&s_addr, (0x0fff | 0x4000), m); /* Host instead of net */
        mask.s_addr = m;
        /* "%15.15s" truncates hostnames, do we really want that? */
        /*
        printf("%-15.15s %-15.15s %-16s%-6s", sdest, sgw, inet_ntoa(mask), flags);
        if (netstatfmt) {
          printf("%5d %-5d %6d %s\n", mtu, win, ir, devname);
        } else {
          printf("%-6d %-2d %7d %s\n", metric, ref, use, devname);
        }
        */
        if(flgs & RTF_GATEWAY)
        {
            fclose(fp);
            return sgw;
        }
    }

    fclose(fp);
    return NULL;
}
