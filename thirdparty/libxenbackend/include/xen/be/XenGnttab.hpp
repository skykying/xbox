/*
 *  Xen gnttab wrapper
 *  Copyright (c) 2016, Oleksandr Grytsov
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef XENBE_XENGNTTAB_HPP_
#define XENBE_XENGNTTAB_HPP_

#include <sys/mman.h>
#include <vector>

extern "C" {
#include <xenctrl.h>
#include <xengnttab.h>
}

#include "Exception.hpp"
#include "Log.hpp"

namespace XenBackend {

typedef std::vector<grant_ref_t> GrantRefs;

/***************************************************************************//**
 * Exception generated by XenGbttab.
 * @ingroup xen
 ******************************************************************************/
class XenGnttabException : public Exception
{
	using Exception::Exception;
};

/***************************************************************************//**
 * Keeps common grant table handle.
 * @ingroup xen
 ******************************************************************************/
class XenGnttab
{
public:
	/**
	 * Returns the grant table handle
	 * @return handle
	 */
	static xengnttab_handle* getHandle();

private:

	friend class XenGnttabBuffer;
	friend class XenGnttabDmaBufferExporter;
	friend class XenGnttabDmaBufferImporter;

	XenGnttab();
	XenGnttab(const XenGnttab&) = delete;
	XenGnttab& operator=(XenGnttab const&) = delete;
	~XenGnttab();

	xengnttab_handle* mHandle;
};

/***************************************************************************//**
 * Gran table buffer.
 * XenGnttabBuffer instance maps grant table reference(s) into local linear
 * buffer when constructed. Then address and size of the linear buffer can
 * be accessible by get() and size() methods.
 * @code
 * XenGnttabBuffer buffer(domId, ref);
 *
 * memcpy(buffer.get(), data, size);
 *
 * ...
 *
 * @endcode
 * @ingroup xen
 ******************************************************************************/
class XenGnttabBuffer
{
public:

	/**
	 * @param[in] domId  domain id
	 * @param[in] ref    grant reference id
	 * @param[in] prot   same flag as in mmap()
	 * @param[in] offset offset of the data inside the buffer
	 */
	XenGnttabBuffer(domid_t domId, grant_ref_t ref,
					int prot = PROT_READ | PROT_WRITE,
					size_t offset = 0);

	/**
	 * @param[in] domId  domain id
	 * @param[in] refs   array of grant reference ids
	 * @param[in] count  number of grant refgerence ids
	 * @param[in] prot   same flag as in mmap()
	 * @param[in] offset offset of the data inside the buffer
	 */
	XenGnttabBuffer(domid_t domId, const grant_ref_t* refs, size_t count,
					int prot = PROT_READ | PROT_WRITE,
					size_t offset = 0);
	XenGnttabBuffer(const XenGnttabBuffer&) = delete;
	XenGnttabBuffer& operator=(XenGnttabBuffer const&) = delete;
	~XenGnttabBuffer();

	/**
	 * Returns pointer to the mapped buffer.
	 */
	void* get() const { return static_cast<uint8_t*>(mBuffer) + mOffset; }

	/**
	 * Returns sizeo of the mapped buffer.
	 */
	size_t size() const { return mCount * XC_PAGE_SIZE; }

private:
	void* mBuffer;
	size_t mOffset;
	xengnttab_handle* mHandle;
	size_t mCount;
	Log mLog;


	void init(domid_t domId, const grant_ref_t* refs, size_t count, int prot,
			  size_t offset);
	void release();
};

/***************************************************************************//**
 * Create a DMA buffer for grant reference(s) provided.
 * XenGnttabDmaBufferExporter maps foreign grant table reference(s)
 * and produces a local DMA buffer. Then the file descriptor of the created
 * local DMA buffer can be accessed by getFd(...) method.
 *
 * @code
 * XenGnttabDmaBufferExporter buffer(1, &refs);
 *
 * int dmaBufFd = buffer.get();
 *
 * @endcode
 * @ingroup xen
 ******************************************************************************/
class XenGnttabDmaBufferExporter
{
public:

	/**
	 * @param[in] foreign domId domain id for which grant references
	 *            will be produced
	 * @param[in] refs  vector of grant reference ids
	 */
	XenGnttabDmaBufferExporter(domid_t domId, const GrantRefs &refs,
							   size_t offset = 0);

	int getFd() const { return mDmaBufFd; }

	int waitForReleased(int timeoutMs);

	XenGnttabDmaBufferExporter(const XenGnttabDmaBufferExporter&) =
			delete;
	XenGnttabDmaBufferExporter& operator=(XenGnttabDmaBufferExporter const&) =
			delete;
	~XenGnttabDmaBufferExporter();

private:
	int mDmaBufFd;
	xengnttab_handle* mHandle;
	Log mLog;

	void init(domid_t domId, const GrantRefs &refs, size_t offset);
	void release();
};

/***************************************************************************//**
 * Grant references for the pages of a DMA buffer.
 * XenGnttabDmaBufferImporter grants reference(s) and exports those for
 * a local DMA buffer provided. Then the grant refereneces of the imported
 * buffer can be accessed by getRefs(...) method.
 *
 * @code
 * XenGnttabDmaBufferImporter buffer(1, dmabuf_fd, 256);
 *
 * GrantRefs refs = buffer.getRefs();
 *
 * @endcode
 * @ingroup xen
 ******************************************************************************/
class XenGnttabDmaBufferImporter
{
public:

	/**
	 * @param[in] domId domain id
	 * @param[in] fd file descriptor of the DMA buffer
	 * @param[in] vector of grant references to hold the results.
	 */
	XenGnttabDmaBufferImporter(domid_t domId, int fd, GrantRefs &refs);

	XenGnttabDmaBufferImporter(const XenGnttabDmaBufferImporter&) =
			delete;
	XenGnttabDmaBufferImporter& operator=(XenGnttabDmaBufferImporter const&) =
			delete;
	~XenGnttabDmaBufferImporter();

	/**
	 * Returns offset of the data of the buffer.
	 */
	size_t offset() const { return mOffset; }

private:
	GrantRefs mRefs;
	int mDmaBufFd;
	xengnttab_handle* mHandle;
	size_t mOffset;
	Log mLog;

	void init(domid_t domId, int fd, GrantRefs &refs);
	void release();
};

}

#endif /* XENBE_XENGNTTAB_HPP_ */
