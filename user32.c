INT APIENTRY NtUserEnumDisplayMonitors(
    OPTIONAL IN HDC hdc,
    OPTIONAL IN LPCRECTL pUnsafeRect,
    OPTIONAL OUT HMONITOR *phUnsafeMonitorList,
    OPTIONAL OUT PRECTL prcUnsafeMonitorList,
    OPTIONAL IN DWORD dwListSize)
{
    UINT cMonitors, i;
    INT iRet = -1;
    HMONITOR *phMonitorList = NULL;
    PRECTL prcMonitorList = NULL;
    RECTL rc, *pRect;
    RECTL DcRect = {0};
    NTSTATUS Status;

    /* Get rectangle */
    if (pUnsafeRect != NULL)
    {
        Status = MmCopyFromCaller(&rc, pUnsafeRect, sizeof(RECT));
        if (!NT_SUCCESS(Status))
        {
            TRACE("MmCopyFromCaller() failed!\n");
            SetLastNtError(Status);
            return -1;
        }
    }

    if (hdc != NULL)
    {
        PDC pDc;
        INT iRgnType;

        /* Get visible region bounding rect */
        pDc = DC_LockDc(hdc);
        if (pDc == NULL)
        {
            TRACE("DC_LockDc() failed!\n");
            /* FIXME: setlasterror? */
            return -1;
        }
        iRgnType = REGION_GetRgnBox(pDc->prgnVis, &DcRect);
        DC_UnlockDc(pDc);

        if (iRgnType == 0)
        {
            TRACE("NtGdiGetRgnBox() failed!\n");
            return -1;
        }
        if (iRgnType == NULLREGION)
            return 0;
        if (iRgnType == COMPLEXREGION)
        {
            /* TODO: Warning */
        }

        /* If hdc and pRect are given the area of interest is pRect with
           coordinate origin at the DC position */
        if (pUnsafeRect != NULL)
        {
            rc.left += DcRect.left;
            rc.right += DcRect.left;
            rc.top += DcRect.top;
            rc.bottom += DcRect.top;
        }
        /* If hdc is given and pRect is not the area of interest is the
           bounding rect of hdc */
        else
        {
            rc = DcRect;
        }
    }

    if (hdc == NULL && pUnsafeRect == NULL)
        pRect = NULL;
    else
        pRect = &rc;

    UserEnterShared();

    /* Find intersecting monitors */
    cMonitors = IntGetMonitorsFromRect(pRect, NULL, NULL, 0, MONITOR_DEFAULTTONULL);
    if (cMonitors == 0 || dwListSize == 0 ||
        (phUnsafeMonitorList == NULL && prcUnsafeMonitorList == NULL))
    {
        /* Simple case - just return monitors count */
        TRACE("cMonitors = %u\n", cMonitors);
        iRet = cMonitors;
        goto cleanup;
    }

    /* Allocate safe buffers */
    if (phUnsafeMonitorList != NULL && dwListSize != 0)
    {
        phMonitorList = ExAllocatePoolWithTag(PagedPool, sizeof (HMONITOR) * dwListSize, USERTAG_MONITORRECTS);
        if (phMonitorList == NULL)
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
    }
    if (prcUnsafeMonitorList != NULL && dwListSize != 0)
    {
        prcMonitorList = ExAllocatePoolWithTag(PagedPool, sizeof(RECT) * dwListSize,USERTAG_MONITORRECTS);
        if (prcMonitorList == NULL)
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
    }

    /* Get intersecting monitors */
    cMonitors = IntGetMonitorsFromRect(pRect, phMonitorList, prcMonitorList,
                                       dwListSize, MONITOR_DEFAULTTONULL);

    if (hdc != NULL && pRect != NULL && prcMonitorList != NULL)
    {
        for (i = 0; i < min(cMonitors, dwListSize); i++)
        {
            _Analysis_assume_(i < dwListSize);
            prcMonitorList[i].left -= DcRect.left;
            prcMonitorList[i].right -= DcRect.left;
            prcMonitorList[i].top -= DcRect.top;
            prcMonitorList[i].bottom -= DcRect.top;
        }
    }

    /* Output result */
    if (phUnsafeMonitorList != NULL && dwListSize != 0)
    {
        Status = MmCopyToCaller(phUnsafeMonitorList, phMonitorList, sizeof(HMONITOR) * dwListSize);
        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            goto cleanup;
        }
    }
    if (prcUnsafeMonitorList != NULL && dwListSize != 0)
    {
        Status = MmCopyToCaller(prcUnsafeMonitorList, prcMonitorList, sizeof(RECT) * dwListSize);
        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            goto cleanup;
        }
    }

    /* Return monitors count on success */
    iRet = cMonitors;

cleanup:
    if (phMonitorList)
        ExFreePoolWithTag(phMonitorList, USERTAG_MONITORRECTS);
    if (prcMonitorList)
        ExFreePoolWithTag(prcMonitorList, USERTAG_MONITORRECTS);

    UserLeave();
    return iRet;
}
