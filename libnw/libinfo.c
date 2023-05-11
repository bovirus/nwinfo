// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"
#include <libcpuid.h>
#include <winring0.h>

static void
PrintDriverVerison(PNODE node, struct wr0_drv_t* drv)
{
	DWORD dwLen;
	LPVOID pBlock = NULL;
	UINT uLen;
	VS_FIXEDFILEINFO *pInfo;
	dwLen = GetFileVersionInfoSizeW(drv->driver_path, NULL);
	if (!dwLen)
		return;
	pBlock = malloc(dwLen);
	if (!pBlock)
		return;
	if (!GetFileVersionInfoW(drv->driver_path, 0, dwLen, pBlock))
		goto fail;
	if (!VerQueryValueA(pBlock, "\\", &pInfo, &uLen))
		goto fail;
	NWL_NodeAttrSetf(node, "Driver Version", 0, "%u.%u.%u.%u",
		(pInfo->dwFileVersionMS >> 16) & 0xffff, pInfo->dwFileVersionMS & 0xffff,
		(pInfo->dwFileVersionLS >> 16) & 0xffff, pInfo->dwFileVersionLS & 0xffff);
fail:
	free(pBlock);
	return;
}

PNODE NW_Libinfo(VOID)
{
	PNODE pNode = NWL_NodeAlloc("LIBINF", NFLG_TABLE);
	if (NWLC->LibInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, pNode);
	NWL_NodeAttrSet(pNode, "Build Time", __DATE__ " " __TIME__, 0);
	NWL_NodeAttrSetf(pNode, "MSVC Version", 0, "%u", _MSC_FULL_VER);
	if (NWLC->NwDrv)
	{
		NWL_NodeAttrSetf(pNode, "Driver", 0, "%S", NWLC->NwDrv->driver_id);
		NWL_NodeAttrSetf(pNode, "Driver Path", 0, "%S", NWLC->NwDrv->driver_path);
		PrintDriverVerison(pNode, NWLC->NwDrv);
	}
	else
		NWL_NodeAttrSet(pNode, "Driver", "NOT FOUND", 0);
	NWL_NodeAttrSetf(pNode, "Language ID", 0, "%u", GetUserDefaultUILanguage());
	NWL_NodeAttrSet(pNode, "Homepage", "https://github.com/a1ive/nwinfo", 0);
	NWL_NodeAttrSet(pNode, "libcpuid", cpuid_lib_version(), 0);
	NWL_NodeAttrSet(pNode, "PCI ID", NWL_GetIdsDate("pci.ids"), 0);
	NWL_NodeAttrSet(pNode, "USB ID", NWL_GetIdsDate("usb.ids"), 0);
	NWL_NodeAttrSet(pNode, "PNP ID", NWL_GetIdsDate("pnp.ids"), 0);
	NWL_NodeAttrSet(pNode, "JEP106 ID", NWL_GetIdsDate("jep106.ids"), 0);
	return pNode;
}
