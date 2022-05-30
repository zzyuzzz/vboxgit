/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)ip_icmp.c   8.2 (Berkeley) 1/4/94
 * ip_icmp.c,v 1.7 1995/05/30 08:09:42 rgrimes Exp
 */

#include "slirp.h"
#include "ip_icmp.h"
#ifdef RT_OS_WINDOWS
#include <Icmpapi.h>
#include <Iphlpapi.h>
#endif


/* The message sent when emulating PING */
/* Be nice and tell them it's just a psuedo-ping packet */
static const char icmp_ping_msg[] = "This is a psuedo-PING packet used by Slirp to emulate ICMP ECHO-REQUEST packets.\n";

/* list of actions for icmp_error() on RX of an icmp message */
static const int icmp_flush[19] = {
/*  ECHO REPLY (0)  */   0,
                         1,
                         1,
/* DEST UNREACH (3) */   1,
/* SOURCE QUENCH (4)*/   1,
/* REDIRECT (5) */       1,
                         1,
                         1,
/* ECHO (8) */           0,
/* ROUTERADVERT (9) */   1,
/* ROUTERSOLICIT (10) */ 1,
/* TIME EXCEEDED (11) */ 1,
/* PARAMETER PROBLEM (12) */ 1,
/* TIMESTAMP (13) */     0,
/* TIMESTAMP REPLY (14) */ 0,
/* INFO (15) */          0,
/* INFO REPLY (16) */    0,
/* ADDR MASK (17) */     0,
/* ADDR MASK REPLY (18) */ 0
};

#ifdef VBOX_WITH_SLIRP_ICMP
int
icmp_init(PNATState pData)
{
        pData->icmp_socket.so_type = IPPROTO_ICMP;
        pData->icmp_socket.so_state = SS_ISFCONNECTED;
#ifndef RT_OS_WINDOWS
        pData->icmp_socket.s = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
#else
        pData->icmp_socket.s = IcmpCreateFile();
    	pData->phEvents[VBOX_ICMP_EVENT_INDEX] = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
        insque(pData, &pData->icmp_socket, &udb);
        LIST_INIT(&pData->icmp_msg_head);
        return (0);
}

/*
 * ip here is ip header + 64bytes readed from ICMP packet
 */
struct icmp_msg *
icmp_find_original_mbuf(PNATState pData, struct ip *ip)
{
    struct mbuf *m0;
    struct ip *ip0;
    struct icmp *icp, *icp0;
    struct icmp_msg *icm;
    int found = 0;
    struct socket *head_socket;
    struct in_addr laddr, faddr;
    u_int lport, fport;

    switch (ip->ip_p) {
    case IPPROTO_ICMP:
        icp = (struct icmp *)((char *)ip + (ip->ip_hl << 2));
        LIST_FOREACH(icm, &pData->icmp_msg_head, im_list)
        {
            m0 = icm->im_m;
            ip0 = mtod(m0, struct ip *);
            AssertRelease(ip0->ip_p == IPPROTO_ICMP);
            icp0 = (struct icmp *)((char *)ip0 + (ip0->ip_hl << 2));
            if (((icp->icmp_type != ICMP_ECHO && ip->ip_src.s_addr == ip0->ip_dst.s_addr)
                ||(icp->icmp_type == ICMP_ECHO && ip->ip_dst.s_addr == ip0->ip_dst.s_addr))
                && icp->icmp_id == icp0->icmp_id
                && icp->icmp_seq == icp->icmp_seq) {
                found = 1;
                break;
            }
        }
    case IPPROTO_UDP:
        head_socket = &udb;
    case IPPROTO_TCP:
        head_socket = (head_socket != NULL ? head_socket : &tcb); /* head_socket could be initialized with udb*/
    }
    if (found == 1) {
        return (icm);
    }
    return (NULL);
}

static int
icmp_attach(PNATState pData, struct mbuf *m) {
    struct icmp_msg *icm;
    struct ip *ip;
    ip = mtod(m, struct ip *);
    Assert(ip->ip_p == IPPROTO_ICMP);
    icm = malloc(sizeof(struct icmp_msg));
    icm->im_m = m;
    LIST_INSERT_HEAD(&pData->icmp_msg_head, icm, im_list);
    return (0);
}
#endif /* VBOX_WITH_SLIRP_ICMP */

/*
 * Process a received ICMP message.
 */
void
icmp_input(PNATState pData, struct mbuf *m, int hlen)
{
  register struct icmp *icp;
  register struct ip *ip=mtod(m, struct ip *);
  int icmplen=ip->ip_len;
  int status;
  /* int code; */

  DEBUG_CALL("icmp_input");
  DEBUG_ARG("m = %lx", (long )m);
  DEBUG_ARG("m_len = %d", m->m_len);

  icmpstat.icps_received++;

  /*
   * Locate icmp structure in mbuf, and check
   * that its not corrupted and of at least minimum length.
   */
  if (icmplen < ICMP_MINLEN) {          /* min 8 bytes payload */
    icmpstat.icps_tooshort++;
  freeit:
    m_freem(pData, m);
    goto end_error;
  }

  m->m_len -= hlen;
  m->m_data += hlen;
  icp = mtod(m, struct icmp *);
  if (cksum(m, icmplen)) {
    icmpstat.icps_checksum++;
    goto freeit;
  }
  m->m_len += hlen;
  m->m_data -= hlen;

  /*    icmpstat.icps_inhist[icp->icmp_type]++; */
  /* code = icp->icmp_code; */

  DEBUG_ARG("icmp_type = %d", icp->icmp_type);
  switch (icp->icmp_type) {
  case ICMP_ECHO:
#ifndef VBOX_WITH_SLIRP_ICMP
    icp->icmp_type = ICMP_ECHOREPLY;
#endif /* !VBOX_WITH_SLIRP_ICMP */

    ip->ip_len += hlen;              /* since ip_input subtracts this */
    if (ip->ip_dst.s_addr == alias_addr.s_addr
    ) {
#ifdef VBOX_WITH_SLIRP_ICMP
      icp->icmp_type = ICMP_ECHOREPLY;
#endif /* VBOX_WITH_SLIRP_ICMP */
      icmp_reflect(pData, m);
    } else {
      struct socket *so;
      struct sockaddr_in addr;
#ifndef VBOX_WITH_SLIRP_ICMP
      if ((so = socreate()) == NULL) goto freeit;
      if(udp_attach(pData, so) == -1)
      {
        DEBUG_MISC((dfd,"icmp_input udp_attach errno = %d-%s\n",
                    errno,strerror(errno)));
        sofree(pData, so);
        m_free(pData, m);
        goto end_error;
      }
      so->so_m = m;
      so->so_faddr = ip->ip_dst;
      so->so_fport = htons(7);
      so->so_laddr = ip->ip_src;
      so->so_lport = htons(9);
      so->so_iptos = ip->ip_tos;
      so->so_type = IPPROTO_ICMP;
      so->so_state = SS_ISFCONNECTED;

      addr.sin_family = AF_INET;
      if ((so->so_faddr.s_addr & htonl(pData->netmask)) == special_addr.s_addr) {
        /* It's an alias */
        switch(ntohl(so->so_faddr.s_addr) & ~pData->netmask) {
        case CTL_DNS:
          addr.sin_addr = dns_addr;
          break;
        case CTL_ALIAS:
        default:
          addr.sin_addr = loopback_addr;
          break;
        }
      } else {
        addr.sin_addr = so->so_faddr;
      }
      addr.sin_port = so->so_fport;
      if(sendto(so->s, icmp_ping_msg, strlen(icmp_ping_msg), 0,
                (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        DEBUG_MISC((dfd,"icmp_input udp sendto tx errno = %d-%s\n",
                    errno,strerror(errno)));
        icmp_error(pData, m, ICMP_UNREACH,ICMP_UNREACH_NET, 0,strerror(errno));
        udp_detach(pData, so);
      }
#else /* !VBOX_WITH_SLIRP_ICMP */
#ifdef RT_OS_WINDOWS      
      IP_OPTION_INFORMATION ipopt;
#endif
      addr.sin_family = AF_INET;
      if ((ip->ip_dst.s_addr & htonl(pData->netmask)) == special_addr.s_addr) {
        /* It's an alias */
        switch(ntohl(ip->ip_dst.s_addr) & ~pData->netmask) {
        case CTL_DNS:
          addr.sin_addr = dns_addr;
          break;
        case CTL_ALIAS:
        default:
          addr.sin_addr = loopback_addr;
          break;
        }
      } else {
            addr.sin_addr.s_addr = ip->ip_dst.s_addr;
      }
      icmp_attach(pData, m);
      /* Send the packet */
#ifndef RT_OS_WINDOWS
      status = setsockopt(pData->icmp_socket.s, IPPROTO_IP, IP_TTL, (void *)&ip->ip_ttl, sizeof(ip->ip_ttl));
      if (status < 0) {
            LogRel(("error(%s) occured while setting TTL attribute of IP packet\n", strerror(errno)));
      }
      if(sendto(pData->icmp_socket.s, icp, icmplen, 0,
                (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        DEBUG_MISC((dfd,"icmp_input udp sendto tx errno = %d-%s\n",
                    errno,strerror(errno)));
        icmp_error(pData, m, ICMP_UNREACH,ICMP_UNREACH_NET, 0,strerror(errno));
        m_free(pData, m);
      }
#else
      memset(&ipopt, 0, sizeof(IP_OPTION_INFORMATION));
      ipopt.Ttl = ip->ip_ttl;
      status = IcmpSendEcho2(pData->icmp_socket.s, pData->phEvents[VBOX_ICMP_EVENT_INDEX], NULL, NULL, &addr, icp, icmplen, &ipopt, pData->pvIcmpBuffer, pData->szIcmpBuffer, 0);
      if (status == 0) {
            LogRel(("error(%d) occured while sending ICMP\n", GetLastError()));
      }
#endif

#endif /* VBOX_WITH_SLIRP_ICMP */
    } /* if ip->ip_dst.s_addr == alias_addr.s_addr */
    break;
  case ICMP_UNREACH:
    /* XXX? report error? close socket? */
  case ICMP_TIMXCEED:
  case ICMP_PARAMPROB:
  case ICMP_SOURCEQUENCH:
  case ICMP_TSTAMP:
  case ICMP_MASKREQ:
  case ICMP_REDIRECT:
    icmpstat.icps_notsupp++;
    m_freem(pData, m);
    break;

  default:
    icmpstat.icps_badtype++;
    m_freem(pData, m);
  } /* swith */

end_error:
  /* m is m_free()'d xor put in a socket xor or given to ip_send */
  return;
}


/*
 *      Send an ICMP message in response to a situation
 *
 *      RFC 1122: 3.2.2 MUST send at least the IP header and 8 bytes of header. MAY send more (we do).
 *                      MUST NOT change this header information.
 *                      MUST NOT reply to a multicast/broadcast IP address.
 *                      MUST NOT reply to a multicast/broadcast MAC address.
 *                      MUST reply to only the first fragment.
 */
/*
 * Send ICMP_UNREACH back to the source regarding msrc.
 * mbuf *msrc is used as a template, but is NOT m_free()'d.
 * It is reported as the bad ip packet.  The header should
 * be fully correct and in host byte order.
 * ICMP fragmentation is illegal.  All machines must accept 576 bytes in one
 * packet.  The maximum payload is 576-20(ip hdr)-8(icmp hdr)=548
 */

#define ICMP_MAXDATALEN (IP_MSS-28)
void icmp_error(PNATState pData, struct mbuf *msrc, u_char type, u_char code, int minsize, char *message)
{
  unsigned hlen, shlen, s_ip_len;
  register struct ip *ip;
  register struct icmp *icp;
  register struct mbuf *m;

  DEBUG_CALL("icmp_error");
  DEBUG_ARG("msrc = %lx", (long )msrc);
  DEBUG_ARG("msrc_len = %d", msrc->m_len);

  if(type!=ICMP_UNREACH && type!=ICMP_TIMXCEED) goto end_error;

  /* check msrc */
  if(!msrc) goto end_error;
  ip = mtod(msrc, struct ip *);
#if DEBUG
  { char bufa[20], bufb[20];
    strcpy(bufa, inet_ntoa(ip->ip_src));
    strcpy(bufb, inet_ntoa(ip->ip_dst));
    DEBUG_MISC((dfd, " %.16s to %.16s\n", bufa, bufb));
  }
#endif
  if(ip->ip_off & IP_OFFMASK) goto end_error;    /* Only reply to fragment 0 */

  shlen=ip->ip_hl << 2;
  s_ip_len=ip->ip_len;
  if(ip->ip_p == IPPROTO_ICMP) {
    icp = (struct icmp *)((char *)ip + shlen);
    /*
     *  Assume any unknown ICMP type is an error. This isn't
     *  specified by the RFC, but think about it..
     */
    if(icp->icmp_type>18 || icmp_flush[icp->icmp_type]) goto end_error;
  }

  /* make a copy */
  if(!(m=m_get(pData))) goto end_error;               /* get mbuf */
  { int new_m_size;
    new_m_size=sizeof(struct ip )+ICMP_MINLEN+msrc->m_len+ICMP_MAXDATALEN;
    if(new_m_size>m->m_size) m_inc(m, new_m_size);
  }
  memcpy(m->m_data, msrc->m_data, msrc->m_len);
  m->m_len = msrc->m_len;                        /* copy msrc to m */

  /* make the header of the reply packet */
  ip  = mtod(m, struct ip *);
  hlen= sizeof(struct ip );     /* no options in reply */

  /* fill in icmp */
  m->m_data += hlen;
  m->m_len -= hlen;

  icp = mtod(m, struct icmp *);

  if(minsize) s_ip_len=shlen+ICMP_MINLEN;   /* return header+8b only */
  else if(s_ip_len>ICMP_MAXDATALEN)         /* maximum size */
    s_ip_len=ICMP_MAXDATALEN;

  m->m_len=ICMP_MINLEN+s_ip_len;        /* 8 bytes ICMP header */

  /* min. size = 8+sizeof(struct ip)+8 */

  icp->icmp_type = type;
  icp->icmp_code = code;
  icp->icmp_id = 0;
  icp->icmp_seq = 0;

  memcpy(&icp->icmp_ip, msrc->m_data, s_ip_len);   /* report the ip packet */
  HTONS(icp->icmp_ip.ip_len);
  HTONS(icp->icmp_ip.ip_id);
  HTONS(icp->icmp_ip.ip_off);

#if DEBUG
  if(message) {           /* DEBUG : append message to ICMP packet */
    int message_len;
    char *cpnt;
    message_len=strlen(message);
    if(message_len>ICMP_MAXDATALEN) message_len=ICMP_MAXDATALEN;
    cpnt=(char *)m->m_data+m->m_len;
    memcpy(cpnt, message, message_len);
    m->m_len+=message_len;
  }
#endif

  icp->icmp_cksum = 0;
  icp->icmp_cksum = cksum(m, m->m_len);

  m->m_data -= hlen;
  m->m_len += hlen;

  /* fill in ip */
  ip->ip_hl = hlen >> 2;
  ip->ip_len = m->m_len;

  ip->ip_tos=((ip->ip_tos & 0x1E) | 0xC0);  /* high priority for errors */

  ip->ip_ttl = MAXTTL;
  ip->ip_p = IPPROTO_ICMP;
  ip->ip_dst = ip->ip_src;    /* ip adresses */
  ip->ip_src = alias_addr;

  (void ) ip_output(pData, (struct socket *)NULL, m);

  icmpstat.icps_reflect++;

end_error:
  return;
}
#undef ICMP_MAXDATALEN

/*
 * Reflect the ip packet back to the source
 */
void
icmp_reflect(PNATState pData, struct mbuf *m)
{
  register struct ip *ip = mtod(m, struct ip *);
  int hlen = ip->ip_hl << 2;
  int optlen = hlen - sizeof(struct ip );
  register struct icmp *icp;

  /*
   * Send an icmp packet back to the ip level,
   * after supplying a checksum.
   */
  m->m_data += hlen;
  m->m_len -= hlen;
  icp = mtod(m, struct icmp *);

  icp->icmp_cksum = 0;
  icp->icmp_cksum = cksum(m, ip->ip_len - hlen);

  m->m_data -= hlen;
  m->m_len += hlen;

#ifndef VBOX_WITH_SLIRP_ICMP
  /* fill in ip */
  if (optlen > 0) {
    /*
     * Strip out original options by copying rest of first
     * mbuf's data back, and adjust the IP length.
     */
    memmove((caddr_t)(ip + 1), (caddr_t)ip + hlen,
            (unsigned )(m->m_len - hlen));
    hlen -= optlen;
    ip->ip_hl = hlen >> 2;
    ip->ip_len -= optlen;
    m->m_len -= optlen;
  }
  ip->ip_ttl = MAXTTL;
  { /* swap */
    struct in_addr icmp_dst;
    icmp_dst = ip->ip_dst;
    ip->ip_dst = ip->ip_src;
    ip->ip_src = icmp_dst;
  }
#endif /* !VBOX_WITH_SLIRP_ICMP */

  (void ) ip_output(pData, (struct socket *)NULL, m);

  icmpstat.icps_reflect++;
}
