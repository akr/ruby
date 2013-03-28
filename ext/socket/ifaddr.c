#include "rubysocket.h"

VALUE rb_cSockIfaddr;

typedef struct rb_ifaddr_tag rb_ifaddr_t;
typedef struct rb_ifaddr_root_tag rb_ifaddr_root_t;

struct rb_ifaddr_tag {
    int ord;
    struct ifaddrs *ifaddr;
    rb_ifaddr_root_t *root;
};

struct rb_ifaddr_root_tag {
    int refcount;
    int numifaddrs;
    rb_ifaddr_t ary[1];
};

static rb_ifaddr_root_t *
get_root(const rb_ifaddr_t *ifaddr)
{
    return (rb_ifaddr_root_t *)((char *)&ifaddr[-ifaddr->ord] -
                                offsetof(rb_ifaddr_root_t, ary));
}

static void
ifaddr_mark(void *ptr)
{
}

static void
ifaddr_free(void *ptr)
{
    rb_ifaddr_t *ifaddr = ptr;
    rb_ifaddr_root_t *root = get_root(ifaddr);
    root->refcount--;
    if (root->refcount == 0) {
/* fprintf(stderr, "freeifaddrs %lx\n", (unsigned long)root->ary[0].ifaddr); */
        freeifaddrs(root->ary[0].ifaddr);
        xfree(root);
    }
}

static size_t
ifaddr_memsize(const void *ptr)
{
    const rb_ifaddr_t *ifaddr;
    const rb_ifaddr_root_t *root;
    if (ptr == NULL)
        return 0;
    ifaddr = ptr;
    root = get_root(ifaddr);
    return sizeof(rb_ifaddr_root_t) + (root->numifaddrs - 1) * sizeof(rb_ifaddr_t);
}

static const rb_data_type_t ifaddr_type = {
    "socket/ifaddr",
    {ifaddr_mark, ifaddr_free, ifaddr_memsize,},
};

#define IS_IFADDRS(obj) rb_typeddata_is_kind_of((obj), &ifaddr_type)
static inline rb_ifaddr_t *
check_ifaddr(VALUE self)
{
      return rb_check_typeddata(self, &ifaddr_type);
}

static rb_ifaddr_t *
get_ifaddr(VALUE self)
{
    rb_ifaddr_t *rifaddr = check_ifaddr(self);

    if (!rifaddr) {
        rb_raise(rb_eTypeError, "uninitialized ifaddr");
    }
    return rifaddr;
}

static VALUE
rsock_getifaddrs(void)
{
    int ret;
    int numifaddrs, i;
    struct ifaddrs *ifaddrs, *ifa;
    rb_ifaddr_root_t *root;
    VALUE result;

    ret = getifaddrs(&ifaddrs);
    if (ret == -1)
        rb_sys_fail("getifaddrs");
/* fprintf(stderr, "getifaddrs %lx\n", (unsigned long)ifaddrs); */

    numifaddrs = 0;
    for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next)
        numifaddrs++;

    root = xmalloc(sizeof(rb_ifaddr_root_t) + (numifaddrs-1) * sizeof(rb_ifaddr_t));
    root->refcount = root->numifaddrs = numifaddrs;

    ifa = ifaddrs;
    for (i = 0; i < numifaddrs; i++) {
        root->ary[i].ord = i;
        root->ary[i].ifaddr = ifa;
        root->ary[i].root = root;
        ifa = ifa->ifa_next;
    }

    result = rb_ary_new2(numifaddrs);
    for (i = 0; i < numifaddrs; i++) {
        rb_ary_push(result, TypedData_Wrap_Struct(rb_cSockIfaddr, &ifaddr_type, &root->ary[i]));
    }

    return result;
}

static VALUE
ifaddr_inspect(VALUE self)
{
    rb_ifaddr_t *rifaddr = get_ifaddr(self);
    struct ifaddrs *ifa;
    VALUE result;
    result = rb_str_new_cstr("#<");

    ifa = rifaddr->ifaddr;

    rb_str_append(result, rb_class_name(CLASS_OF(self)));
    rb_str_cat2(result, " ");
    rb_str_cat2(result, ifa->ifa_name);
    rb_str_catf(result, " %#x", ifa->ifa_flags);
    if (ifa->ifa_addr) {
      rb_str_cat2(result, " [");
      rsock_inspect_sockaddr(ifa->ifa_addr,
          rsock_sockaddr_len(ifa->ifa_addr),
          result);
      rb_str_cat2(result, "]");
    }
    if (ifa->ifa_netmask) {
      rb_str_cat2(result, " netmask:[");
      rsock_inspect_sockaddr(ifa->ifa_netmask,
          rsock_sockaddr_len(ifa->ifa_netmask),
          result);
      rb_str_cat2(result, "]");
    }

    if ((ifa->ifa_flags & IFF_BROADCAST) && ifa->ifa_broadaddr) {
      rb_str_cat2(result, " broadcast:[");
      rsock_inspect_sockaddr(ifa->ifa_broadaddr,
          rsock_sockaddr_len(ifa->ifa_broadaddr),
          result);
      rb_str_cat2(result, "]");
    }

    if ((ifa->ifa_flags & IFF_POINTOPOINT) && ifa->ifa_dstaddr) {
      rb_str_cat2(result, " dstaddr:[");
      rsock_inspect_sockaddr(ifa->ifa_dstaddr,
          rsock_sockaddr_len(ifa->ifa_dstaddr),
          result);
      rb_str_cat2(result, "]");
    }

    rb_str_cat2(result, ">");
    return result;
}

static VALUE
socket_s_getifaddrs(VALUE self)
{
    return rsock_getifaddrs();
}

void
rsock_init_sockifaddr(void)
{
    /*
     * Document-class: Socket::Ifaddr
     *
     * Socket::Ifaddr represents a result of getifaddrs() function.
     */
    rb_cSockIfaddr = rb_define_class_under(rb_cSocket, "Ifaddr", rb_cData);
    rb_define_method(rb_cSockIfaddr, "inspect", ifaddr_inspect, 0);

    rb_define_singleton_method(rb_cSocket, "getifaddrs", socket_s_getifaddrs, 0);
}
