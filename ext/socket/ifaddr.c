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

/*
 * call-seq:
 *   getifaddr.name => string
 *
 * Returns the interface name of _getifaddr_.
 */

static VALUE
ifaddr_name(VALUE self)
{
    rb_ifaddr_t *rifaddr = get_ifaddr(self);
    struct ifaddrs *ifa = rifaddr->ifaddr;
    return rb_str_new_cstr(ifa->ifa_name);
}

/*
 * call-seq:
 *   getifaddr.ifindex => integer
 *
 * Returns the interface index of _getifaddr_.
 */

static VALUE
ifaddr_ifindex(VALUE self)
{
    rb_ifaddr_t *rifaddr = get_ifaddr(self);
    struct ifaddrs *ifa = rifaddr->ifaddr;
    unsigned int ifindex = if_nametoindex(ifa->ifa_name);
    if (ifindex == 0) {
        rb_raise(rb_eArgError, "invalid interface name: %s", ifa->ifa_name);
    }
    return UINT2NUM(ifindex);
}

/*
 * call-seq:
 *   getifaddr.flags => integer
 *
 * Returns the flags of _getifaddr_.
 */

static VALUE
ifaddr_flags(VALUE self)
{
    rb_ifaddr_t *rifaddr = get_ifaddr(self);
    struct ifaddrs *ifa = rifaddr->ifaddr;
    return UINT2NUM(ifa->ifa_flags);
}

/*
 * call-seq:
 *   getifaddr.addr => addrinfo
 *
 * Returns the address of _getifaddr_.
 * nil is returned if address is not available in _getifaddr_.
 */

static VALUE
ifaddr_addr(VALUE self)
{
    rb_ifaddr_t *rifaddr = get_ifaddr(self);
    struct ifaddrs *ifa = rifaddr->ifaddr;
    if (ifa->ifa_addr)
        return rsock_sockaddr_obj(ifa->ifa_addr, rsock_sockaddr_len(ifa->ifa_addr));
    return Qnil;
}

/*
 * call-seq:
 *   getifaddr.netmask => addrinfo
 *
 * Returns the netmask address of _getifaddr_.
 * nil is returned if netmask is not available in _getifaddr_.
 */

static VALUE
ifaddr_netmask(VALUE self)
{
    rb_ifaddr_t *rifaddr = get_ifaddr(self);
    struct ifaddrs *ifa = rifaddr->ifaddr;
    if (ifa->ifa_netmask)
        return rsock_sockaddr_obj(ifa->ifa_netmask, rsock_sockaddr_len(ifa->ifa_netmask));
    return Qnil;
}

/*
 * call-seq:
 *   getifaddr.broadaddr => addrinfo
 *
 * Returns the broadcast address of _getifaddr_.
 * nil is returned if the flags doesn't have IFF_BROADCAST.
 */

static VALUE
ifaddr_broadaddr(VALUE self)
{
    rb_ifaddr_t *rifaddr = get_ifaddr(self);
    struct ifaddrs *ifa = rifaddr->ifaddr;
    if ((ifa->ifa_flags & IFF_BROADCAST) && ifa->ifa_broadaddr)
        return rsock_sockaddr_obj(ifa->ifa_broadaddr, rsock_sockaddr_len(ifa->ifa_broadaddr));
    return Qnil;
}

/*
 * call-seq:
 *   getifaddr.dstaddr => addrinfo
 *
 * Returns the destination address of _getifaddr_.
 * nil is returned if the flags doesn't have IFF_POINTOPOINT.
 */

static VALUE
ifaddr_dstaddr(VALUE self)
{
    rb_ifaddr_t *rifaddr = get_ifaddr(self);
    struct ifaddrs *ifa = rifaddr->ifaddr;
    if ((ifa->ifa_flags & IFF_POINTOPOINT) && ifa->ifa_dstaddr)
        return rsock_sockaddr_obj(ifa->ifa_dstaddr, rsock_sockaddr_len(ifa->ifa_dstaddr));
    return Qnil;
}

static void
ifaddr_inspect_flags(unsigned int flags, VALUE result)
{
    char *sep = " ";
#define INSPECT_BIT(bit, name) \
    if (flags & (bit)) { rb_str_catf(result, "%s" name, sep); flags &= ~(bit); sep = ","; }
#ifdef IFF_UP
    INSPECT_BIT(IFF_UP, "UP")
#endif
#ifdef IFF_BROADCAST
    INSPECT_BIT(IFF_BROADCAST, "BROADCAST")
#endif
#ifdef IFF_DEBUG
    INSPECT_BIT(IFF_DEBUG, "DEBUG")
#endif
#ifdef IFF_LOOPBACK
    INSPECT_BIT(IFF_LOOPBACK, "LOOPBACK")
#endif
#ifdef IFF_POINTOPOINT
    INSPECT_BIT(IFF_POINTOPOINT, "POINTOPOINT")
#endif
#ifdef IFF_RUNNING
    INSPECT_BIT(IFF_RUNNING, "RUNNING")
#endif
#ifdef IFF_NOARP
    INSPECT_BIT(IFF_NOARP, "NOARP")
#endif
#ifdef IFF_PROMISC
    INSPECT_BIT(IFF_PROMISC, "PROMISC")
#endif
#ifdef IFF_NOTRAILERS
    INSPECT_BIT(IFF_NOTRAILERS, "NOTRAILERS")
#endif
#ifdef IFF_ALLMULTI
    INSPECT_BIT(IFF_ALLMULTI, "ALLMULTI")
#endif
#ifdef IFF_MASTER
    INSPECT_BIT(IFF_MASTER, "MASTER")
#endif
#ifdef IFF_SLAVE
    INSPECT_BIT(IFF_SLAVE, "SLAVE")
#endif
#ifdef IFF_MULTICAST
    INSPECT_BIT(IFF_MULTICAST, "MULTICAST")
#endif
#ifdef IFF_PORTSEL
    INSPECT_BIT(IFF_PORTSEL, "PORTSEL")
#endif
#ifdef IFF_AUTOMEDIA
    INSPECT_BIT(IFF_AUTOMEDIA, "AUTOMEDIA")
#endif
#ifdef IFF_DYNAMIC
    INSPECT_BIT(IFF_DYNAMIC, "DYNAMIC")
#endif
#ifdef IFF_LOWER_UP
    INSPECT_BIT(IFF_LOWER_UP, "LOWER_UP")
#endif
#ifdef IFF_DORMANT
    INSPECT_BIT(IFF_DORMANT, "DORMANT")
#endif
#ifdef IFF_ECHO
    INSPECT_BIT(IFF_ECHO, "ECHO")
#endif
#undef INSPECT_BIT
    if (flags) {
        rb_str_catf(result, "%s%#x", sep, flags);
    }
}

/*
 * call-seq:
 *   getifaddr.inspect => string
 *
 * Returns a string to show contents of _getifaddr_.
 */

static VALUE
ifaddr_inspect(VALUE self)
{
    rb_ifaddr_t *rifaddr = get_ifaddr(self);
    struct ifaddrs *ifa;
    VALUE result;

    ifa = rifaddr->ifaddr;

    result = rb_str_new_cstr("#<");

    rb_str_append(result, rb_class_name(CLASS_OF(self)));
    rb_str_cat2(result, " ");
    rb_str_cat2(result, ifa->ifa_name);

    if (ifa->ifa_flags)
        ifaddr_inspect_flags(ifa->ifa_flags, result);

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

/*
 * call-seq:
 *   Socket.getifaddrs => [ifaddr1, ...]
 *
 * Returns an array of interface addresses.
 * An element of the array is an instance of Socket::Ifaddr.
 *
 */

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
    rb_define_method(rb_cSockIfaddr, "name", ifaddr_name, 0);
    rb_define_method(rb_cSockIfaddr, "ifindex", ifaddr_ifindex, 0);
    rb_define_method(rb_cSockIfaddr, "flags", ifaddr_flags, 0);
    rb_define_method(rb_cSockIfaddr, "addr", ifaddr_addr, 0);
    rb_define_method(rb_cSockIfaddr, "netmask", ifaddr_netmask, 0);
    rb_define_method(rb_cSockIfaddr, "broadaddr", ifaddr_broadaddr, 0);
    rb_define_method(rb_cSockIfaddr, "dstaddr", ifaddr_dstaddr, 0);

    rb_define_singleton_method(rb_cSocket, "getifaddrs", socket_s_getifaddrs, 0);
}
