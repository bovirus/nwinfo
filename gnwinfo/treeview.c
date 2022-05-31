// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

VOID
GNW_TreeInit(VOID)
{
	PNODE node;
	INT i, count;
	TreeView_SetImageList(GetDlgItem(GNWC.hWnd, IDC_MAIN_TREE), GNWC.hImageList, TVSIL_NORMAL);

	GNWC.htRoot = GNW_TreeAdd(NULL, "NWinfo", 1, IDI_ICON_TVD_PC, NULL);

	GNWC.htAcpi = GNW_TreeAdd(GNWC.htRoot, "ACPI Table", 2, IDI_ICON_TVN_ACPI, NULL);
	count = NWL_NodeChildCount(GNWC.pnAcpi);
	for (i = 0; i < count; i++)
	{
		LPSTR name;
		node = GNWC.pnAcpi->Children[i].LinkedNode;
		name = NWL_NodeAttrGet(node, "Signature");
		if (!name)
			continue;
		GNW_TreeAdd(GNWC.htAcpi, name, 3, GNW_IconFromAcpi(node, name), node);
	}

	GNWC.htCpuid = GNW_TreeAdd(GNWC.htRoot, "Processor", 2, IDI_ICON_TVN_CPU, GNWC.pnCpuid);
	count = NWL_NodeChildCount(GNWC.pnCpuid);
	for (i = 0; i < count; i++)
	{
		node = GNWC.pnCpuid->Children[i].LinkedNode;
		GNW_TreeAdd(GNWC.htCpuid, node->Name, 3, IDI_ICON_TVN_CPU, node);
	}

	GNWC.htDisk = GNW_TreeAdd(GNWC.htRoot, "Physical Storage", 2, IDI_ICON_TVN_DISK, NULL);
	count = NWL_NodeChildCount(GNWC.pnDisk);
	for (i = 0; i < count; i++)
	{
		HTREEITEM disk;
		LPSTR path;
		INT icon;
		node = GNWC.pnDisk->Children[i].LinkedNode;
		path = NWL_NodeAttrGet(node, "Path");
		if (!path)
			continue;
		icon = GNW_IconFromDisk(node, path);
		disk = GNW_TreeAdd(GNWC.htDisk, path, 3, icon, node);
		if (node->Children[0].LinkedNode)
		{
			PNODE tab;
			INT j, tab_count;
			LPSTR letter;
			tab_count = NWL_NodeChildCount(node->Children[0].LinkedNode);
			for (j = 0; j < tab_count; j++)
			{
				tab = node->Children[0].LinkedNode->Children[j].LinkedNode;
				letter = NWL_NodeAttrGet(tab, "Drive Letter");
				if (!letter)
					continue;
				GNW_TreeAdd(disk, letter, 4, icon, tab);
			}
		}
	}

	GNWC.htEdid = GNW_TreeAdd(GNWC.htRoot, "Display Devices", 2, IDI_ICON_TVN_EDID, NULL);
	count = NWL_NodeChildCount(GNWC.pnEdid);
	for (i = 0; i < count; i++)
	{
		LPSTR hwid;
		node = GNWC.pnEdid->Children[i].LinkedNode;
		hwid = NWL_NodeAttrGet(node, "ID");
		if (!hwid)
			continue;
		GNW_TreeAdd(GNWC.htEdid, hwid, 3, IDI_ICON_TVN_EDID, node);
	}

	GNWC.htNetwork = GNW_TreeAdd(GNWC.htRoot, "Network Adapter", 2, IDI_ICON_TVN_NET, NULL);
	count = NWL_NodeChildCount(GNWC.pnNetwork);
	for (i = 0; i < count; i++)
	{
		LPSTR name;
		node = GNWC.pnNetwork->Children[i].LinkedNode;
		name = NWL_NodeAttrGet(node, "Description");
		if (!name)
			name = NWL_NodeAttrGet(node, "Network Adapter");
		GNW_TreeAdd(GNWC.htNetwork, name, 3, GNW_IconFromNetwork(node, name), node);
	}

	GNWC.htPci = GNW_TreeAdd(GNWC.htRoot, "PCI Devices", 2, IDI_ICON_TVN_PCI, NULL);
	count = NWL_NodeChildCount(GNWC.pnPci);
	for (i = 0; i < count; i++)
	{
		LPSTR hwid;
		node = GNWC.pnPci->Children[i].LinkedNode;
		hwid = NWL_NodeAttrGet(node, "HWID");
		if (!hwid)
			continue;
		GNW_TreeAdd(GNWC.htPci, hwid, 3, GNW_IconFromPci(node, hwid), node);
	}

	node = NULL;
	count = NWL_NodeChildCount(GNWC.pnSmbios);
	for (i = 0; i < count; i++)
	{
		node = GNWC.pnSmbios->Children[i].LinkedNode;
		if (NWL_NodeAttrGet(node, "SMBIOS Version"))
			break;
	}
	GNWC.htSmbios = GNW_TreeAdd(GNWC.htRoot, "SMBIOS", 2, IDI_ICON_TVN_DMI, node);
	for (i = 0; i < count; i++)
	{
		
		LPSTR name, type;
		CHAR tmp[] = "Type 1024";
		node = GNWC.pnSmbios->Children[i].LinkedNode;
		type = NWL_NodeAttrGet(node, "Table Type");
		if (!type)
			continue;
		name = NWL_NodeAttrGet(node, "Description");
		if (!name)
		{
			snprintf(tmp, sizeof(tmp), "Type %s", type);
			name = tmp;
		}
		GNW_TreeAdd(GNWC.htSmbios, name, 3, GNW_IconFromSmbios(node, type), node);
	}

	GNWC.htSpd = GNW_TreeAdd(GNWC.htRoot, "Memory SPD", 2, IDI_ICON_TVN_SPD, NULL);
	count = NWL_NodeChildCount(GNWC.pnSpd);
	for (i = 0; i < count; i++)
	{
		LPSTR mt;
		node = GNWC.pnSpd->Children[i].LinkedNode;
		mt = NWL_NodeAttrGet(node, "Memory Type");
		if (!mt)
			continue;
		GNW_TreeAdd(GNWC.htSpd, mt, 3, IDI_ICON_TVN_SPD, node);
	}

	GNWC.htSystem = GNW_TreeAdd(GNWC.htRoot, "Operating System", 2, IDI_ICON_TVN_SYS, GNWC.pnSystem);

	GNWC.htUsb = GNW_TreeAdd(GNWC.htRoot, "USB Devices", 2, IDI_ICON_TVN_USB, NULL);
	count = NWL_NodeChildCount(GNWC.pnUsb);
	for (i = 0; i < count; i++)
	{
		LPSTR hwid;
		node = GNWC.pnUsb->Children[i].LinkedNode;
		hwid = NWL_NodeAttrGet(node, "HWID");
		if (!hwid)
			continue;
		GNW_TreeAdd(GNWC.htUsb, hwid, 3, IDI_ICON_TVD_USB, node);
	}
}

HTREEITEM
GNW_TreeAdd(HTREEITEM hParent, LPCSTR lpszItem, INT nLevel, INT nIcon, LPVOID lpConfig)
{
	TVITEMA tvi;
	TVINSERTSTRUCTA tvins;
	static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
	HTREEITEM hti;
	HWND hwndTV = GetDlgItem(GNWC.hWnd, IDC_MAIN_TREE);

	if (!hwndTV || (nLevel != 1 && !hParent) || !lpszItem)
		return NULL;
	tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvi.iImage = (nIcon >= IDI_ICON) ? (nIcon - IDI_ICON) : 0;
	tvi.iSelectedImage = tvi.iImage;
	tvi.pszText = GNW_GetText(lpszItem);
	tvi.cchTextMax = (int)(strlen(tvi.pszText) + 1);
	tvi.lParam = (LPARAM)lpConfig;
	tvins.item = tvi;
	tvins.hInsertAfter = hPrev;
	if (nLevel == 1)
		tvins.hParent = TVI_ROOT;
	else
		tvins.hParent = hParent;
	hPrev = (HTREEITEM)SendMessageA(hwndTV, TVM_INSERTITEMA,
		0, (LPARAM)(LPTVINSERTSTRUCTA)&tvins);

	if (hPrev == NULL)
		return NULL;

	if (nLevel > 1)
	{
		hti = TreeView_GetParent(hwndTV, hPrev);
		tvi.mask = 0;
		tvi.hItem = hti;
		TreeView_SetItem(hwndTV, &tvi);
	}

	return hPrev;
}

VOID GNW_TreeExpand(HTREEITEM hTree)
{
	HWND hwndTV = GetDlgItem(GNWC.hWnd, IDC_MAIN_TREE);
	if (hwndTV)
		TreeView_Expand(hwndTV, hTree, TVE_EXPAND);
}

VOID GNW_TreeDelete(HTREEITEM hTree)
{
	HWND hwndTV = GetDlgItem(GNWC.hWnd, IDC_MAIN_TREE);
	if (hwndTV)
		TreeView_DeleteItem(hwndTV, hTree);
}

INT_PTR
GNW_TreeUpdate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	LPNMTREEVIEWA pnmtv = (LPNMTREEVIEWA)lParam;
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(hWnd);
	HWND hwndTV = GetDlgItem(hWnd, IDC_MAIN_TREE);
	if (!hwndTV || pnmtv->hdr.code != (UINT)TVN_SELCHANGINGA)
		return (INT_PTR)FALSE;
	GNW_ListClean();
	if (pnmtv->itemNew.lParam)
	{
		BOOL bSkipChild = FALSE;
		HTREEITEM htParent = TreeView_GetParent(hwndTV, pnmtv->itemNew.hItem);
		if (htParent == GNWC.htDisk)
			bSkipChild = TRUE;
		else if (pnmtv->itemNew.hItem == GNWC.htCpuid)
			bSkipChild = TRUE;
		GNW_ListAdd((PNODE)pnmtv->itemNew.lParam, bSkipChild);
	}
	GNW_TreeExpand(pnmtv->itemNew.hItem);
	return (INT_PTR)TRUE;
}
