/*
 *  linux/include/asm-arm/io.h
 *
 *  Copyright (C) 1996-2000 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Modifications:
 *  16-Sep-1996	RMK	Inlined the inx/outx functions & optimised for both
 *			constant addresses and variable addresses.
 *  04-Dec-1997	RMK	Moved a lot of this stuff to the new architecture
 *			specific IO header files.
 *  27-Mar-1999	PJB	Second parameter of memcpy_toio is const..
 *  04-Apr-1999	PJB	Added check_signature.
 *  12-Dec-1999	RMK	More cleanups
 *  18-Jun-2000 RMK	Removed virt_to_* and friends definitions
 */
#ifndef __ASM_ARM_IO_H
#define __ASM_ARM_IO_H

#ifdef __KERNEL__

#include <linux/types.h>
#include <asm/byteorder.h>
#include <asm/memory.h>

static inline void sync(void)
{
}

/*
 * Given a physical address and a length, return a virtual address
 * that can be used to access the memory range with the caching
 * properties specified by "flags".
 */
#define MAP_NOCACHE	(0)
#define MAP_WRCOMBINE	(0)
#define MAP_WRBACK	(0)
#define MAP_WRTHROUGH	(0)

static inline void *
map_physmem(phys_addr_t paddr, unsigned long len, unsigned long flags)
{
	return (void *)paddr;
}

/*
 * Take down a mapping set up by map_physmem().
 */
static inline void unmap_physmem(void *vaddr, unsigned long flags)
{

}

static inline phys_addr_t virt_to_phys(void * vaddr)
{
	return (phys_addr_t)(vaddr);
}

/*
 * Generic virtual read/write.  Note that we don't support half-word
 * read/writes.  We define __arch_*[bl] here, and leave __arch_*w
 * to the architecture specific code.
 */
#define __arch_getb(a)			(*(volatile unsigned char *)(a))
#define __arch_getw(a)			(*(volatile unsigned short *)(a))
#define __arch_getl(a)			(*(volatile unsigned int *)(a))

#define __arch_putb(v,a)		(*(volatile unsigned char *)(a) = (v))
#define __arch_putw(v,a)		(*(volatile unsigned short *)(a) = (v))
#define __arch_putl(v,a)		(*(volatile unsigned int *)(a) = (v))

extern inline void __raw_writesb(unsigned int addr, const void *data, int bytelen)
{
	uint8_t *buf = (uint8_t *)data;
	while(bytelen--)
		__arch_putb(*buf++, addr);
}

extern inline void __raw_writesw(unsigned int addr, const void *data, int wordlen)
{
	uint16_t *buf = (uint16_t *)data;
	while(wordlen--)
		__arch_putw(*buf++, addr);
}

extern inline void __raw_writesl(unsigned int addr, const void *data, int longlen)
{
	uint32_t *buf = (uint32_t *)data;
	while(longlen--)
		__arch_putl(*buf++, addr);
}

extern inline void __raw_readsb(unsigned int addr, void *data, int bytelen)
{
	uint8_t *buf = (uint8_t *)data;
	while(bytelen--)
		*buf++ = __arch_getb(addr);
}

extern inline void __raw_readsw(unsigned int addr, void *data, int wordlen)
{
	uint16_t *buf = (uint16_t *)data;
	while(wordlen--)
		*buf++ = __arch_getw(addr);
}

extern inline void __raw_readsl(unsigned int addr, void *data, int longlen)
{
	uint32_t *buf = (uint32_t *)data;
	while(longlen--)
		*buf++ = __arch_getl(addr);
}

#define __raw_writeb(v,a)	__arch_putb(v,a)
#define __raw_writew(v,a)	__arch_putw(v,a)
#define __raw_writel(v,a)	__arch_putl(v,a)

#define __raw_readb(a)		__arch_getb(a)
#define __raw_readw(a)		__arch_getw(a)
#define __raw_readl(a)		__arch_getl(a)

/*
 * TODO: The kernel offers some more advanced versions of barriers, it might
 * have some advantages to use them instead of the simple one here.
 */
#define dmb()		__asm__ __volatile__ ("" : : : "memory")
#define __iormb()	dmb()
#define __iowmb()	dmb()

#define writeb(v,c)	({ u8  __v = v; __iowmb(); __arch_putb(__v,c); __v; })
#define writew(v,c)	({ u16 __v = v; __iowmb(); __arch_putw(__v,c); __v; })
#define writel(v,c)	({ u32 __v = v; __iowmb(); __arch_putl(__v,c); __v; })
// #error
#define readb(c)	({ u8  __v = __arch_getb(c); __iormb(); __v; })
#define readw(c)	({ u16 __v = __arch_getw(c); __iormb(); __v; })
#define readl(c)	({ u32 __v = __arch_getl(c); __iormb(); __v; })

/*
 * The compiler seems to be incapable of optimising constants
 * properly.  Spell it out to the compiler in some cases.
 * These are only valid for small values of "off" (< 1<<12)
 */
#define __raw_base_writeb(val,base,off)	__arch_base_putb(val,base,off)
#define __raw_base_writew(val,base,off)	__arch_base_putw(val,base,off)
#define __raw_base_writel(val,base,off)	__arch_base_putl(val,base,off)

#define __raw_base_readb(base,off)	__arch_base_getb(base,off)
#define __raw_base_readw(base,off)	__arch_base_getw(base,off)
#define __raw_base_readl(base,off)	__arch_base_getl(base,off)

/*
 * Clear and set bits in one shot. These macros can be used to clear and
 * set multiple bits in a register using a single call. These macros can
 * also be used to set a multiple-bit bit pattern using a mask, by
 * specifying the mask in the 'clear' parameter and the new bit pattern
 * in the 'set' parameter.
 */

#define out_arch(type,endian,a,v)	__raw_write##type(cpu_to_##endian(v),a)
#define in_arch(type,endian,a)		endian##_to_cpu(__raw_read##type(a))

#define out_le32(a,v)	out_arch(l,le32,a,v)
#define out_le16(a,v)	out_arch(w,le16,a,v)

#define in_le32(a)	in_arch(l,le32,a)
#define in_le16(a)	in_arch(w,le16,a)

#define out_be32(a,v)	out_arch(l,be32,a,v)
#define out_be16(a,v)	out_arch(w,be16,a,v)

#define in_be32(a)	in_arch(l,be32,a)
#define in_be16(a)	in_arch(w,be16,a)

#define out_8(a,v)	__raw_writeb(v,a)
#define in_8(a)		__raw_readb(a)

#define clrbits(type, addr, clear) \
	out_##type((addr), in_##type(addr) & ~(clear))

#define setbits(type, addr, set) \
	out_##type((addr), in_##type(addr) | (set))

#define clrsetbits(type, addr, clear, set) \
	out_##type((addr), (in_##type(addr) & ~(clear)) | (set))

#define clrbits_be32(addr, clear) clrbits(be32, addr, clear)
#define setbits_be32(addr, set) setbits(be32, addr, set)
#define clrsetbits_be32(addr, clear, set) clrsetbits(be32, addr, clear, set)

#define clrbits_le32(addr, clear) clrbits(le32, addr, clear)
#define setbits_le32(addr, set) setbits(le32, addr, set)
#define clrsetbits_le32(addr, clear, set) clrsetbits(le32, addr, clear, set)

#define clrbits_be16(addr, clear) clrbits(be16, addr, clear)
#define setbits_be16(addr, set) setbits(be16, addr, set)
#define clrsetbits_be16(addr, clear, set) clrsetbits(be16, addr, clear, set)

#define clrbits_le16(addr, clear) clrbits(le16, addr, clear)
#define setbits_le16(addr, set) setbits(le16, addr, set)
#define clrsetbits_le16(addr, clear, set) clrsetbits(le16, addr, clear, set)

#define clrbits_8(addr, clear) clrbits(8, addr, clear)
#define setbits_8(addr, set) setbits(8, addr, set)
#define clrsetbits_8(addr, clear, set) clrsetbits(8, addr, clear, set)

#define outb_p(val,port)		outb((val),(port))
#define outw_p(val,port)		outw((val),(port))
#define outl_p(val,port)		outl((val),(port))
#define inb_p(port)			inb((port))
#define inw_p(port)			inw((port))
#define inl_p(port)			inl((port))

#define outsb_p(port,from,len)		outsb(port,from,len)
#define outsw_p(port,from,len)		outsw(port,from,len)
#define outsl_p(port,from,len)		outsl(port,from,len)
#define insb_p(port,to,len)		insb(port,to,len)
#define insw_p(port,to,len)		insw(port,to,len)
#define insl_p(port,to,len)		insl(port,to,len)

/*
 * ioremap and friends.
 *
 * ioremap takes a PCI memory address, as specified in
 * linux/Documentation/IO-mapping.txt.  If you want a
 * physical address, use __ioremap instead.
 */
extern void * __ioremap(unsigned long offset, size_t size, unsigned long flags);
extern void __iounmap(void *addr);

#define ioremap(off,sz)			__arch_ioremap((off),(sz),0)
#define ioremap_nocache(off,sz)		__arch_ioremap((off),(sz),1)
#define iounmap(_addr)			__arch_iounmap(_addr)

/*
 * DMA-consistent mapping functions.  These allocate/free a region of
 * uncached, unwrite-buffered mapped memory space for use with DMA
 * devices.  This is the "generic" version.  The PCI specific version
 * is in pci.h
 */
extern void *consistent_alloc(int gfp, size_t size, dma_addr_t *handle);
extern void consistent_free(void *vaddr, size_t size, dma_addr_t handle);
extern void consistent_sync(void *vaddr, size_t size, int rw);

/*
 * String version of IO memory access ops:
 */
extern void _memcpy_fromio(void *, unsigned long, size_t);
extern void _memcpy_toio(unsigned long, const void *, size_t);
extern void _memset_io(unsigned long, int, size_t);

extern void __readwrite_bug(const char *fn);

#define isa_readb(addr)			(__readwrite_bug("isa_readb"),0)
#define isa_readw(addr)			(__readwrite_bug("isa_readw"),0)
#define isa_readl(addr)			(__readwrite_bug("isa_readl"),0)
#define isa_writeb(val,addr)		__readwrite_bug("isa_writeb")
#define isa_writew(val,addr)		__readwrite_bug("isa_writew")
#define isa_writel(val,addr)		__readwrite_bug("isa_writel")
#define isa_memset_io(a,b,c)		__readwrite_bug("isa_memset_io")
#define isa_memcpy_fromio(a,b,c)	__readwrite_bug("isa_memcpy_fromio")
#define isa_memcpy_toio(a,b,c)		__readwrite_bug("isa_memcpy_toio")

#define isa_eth_io_copy_and_sum(a,b,c,d) \
				__readwrite_bug("isa_eth_io_copy_and_sum")

#define isa_check_signature(io,sig,len)	(0)

#endif	/* __KERNEL__ */
#endif	/* __ASM_ARM_IO_H */
