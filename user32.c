BOOL WINAPI EnumDisplayMonitors(
    HDC hdc,
    LPCRECT lprcClip,
    MONITORENUMPROC lpfnEnum,
    LPARAM dwData)
{
    INT iCount, i;
    HMONITOR *hMonitorList;
    LPRECT pRectList;
    HANDLE hHeap;
    BOOL ret = FALSE;

    /* get list of monitors/rects */
    iCount = EnumDisplayMonitors(hdc, lprcClip, NULL, NULL, 0);
    if (iCount < 0)
    {
        /* FIXME: SetLastError() */
        return FALSE;
    }
    if (iCount == 0)
    {
        return TRUE;
    }

    hHeap = GetProcessHeap();
    hMonitorList = HeapAlloc(hHeap, 0, sizeof (HMONITOR) * iCount);
    if (hMonitorList == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    pRectList = HeapAlloc(hHeap, 0, sizeof (RECT) * iCount);
    if (pRectList == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }

    iCount = EnumDisplayMonitors(hdc, lprcClip, hMonitorList, pRectList, iCount);
    if (iCount <= 0)
    {
        /* FIXME: SetLastError() */
        goto cleanup;
    }

    /* enumerate list */
    for (i = 0; i < iCount; i++)
    {
        HMONITOR hMonitor = hMonitorList[i];
        LPRECT pMonitorRect = pRectList + i;
        HDC hMonitorDC = NULL;

        if (hdc != NULL)
        {
            /* make monitor DC */
            hMonitorDC = hdc;
        }

        if (!lpfnEnum(hMonitor, hMonitorDC, pMonitorRect, dwData))
            goto cleanup; /* return FALSE */
    }
    
    ret = TRUE;
    
cleanup:
    if(hMonitorList)
        HeapFree(hHeap, 0, hMonitorList);
    if(pRectList)
        HeapFree(hHeap, 0, pRectList);
    return ret;
}

DWORD WINAPI GetGuiResources(HANDLE hProcess, DWORD uiFlags)
{
   PEPROCESS Process;
   PPROCESSINFO W32Process;
   NTSTATUS Status;
   DWORD Ret = 0;
   TRACE("Enter GetGuiResources\n");
   UserEnterShared();
   Status = ObReferenceObjectByHandle(hProcess,
                                      PROCESS_QUERY_INFORMATION,
                                      *PsProcessType,
                                      ExGetPreviousMode(),
                                      (PVOID*)&Process,
                                      NULL);
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      goto Exit; // Return 0
   }
   W32Process = (PPROCESSINFO)Process->Win32Process;
   if(!W32Process)
   {
      ObDereferenceObject(Process);
      EngSetLastError(ERROR_INVALID_PARAMETER);
      goto Exit; // Return 0
   }
   switch(uiFlags)
   {
      case GR_GDIOBJECTS:
         {
            Ret = (DWORD)W32Process->GDIHandleCount;
            break;
         }
      case GR_USEROBJECTS:
         {
            Ret = (DWORD)W32Process->UserHandleCount;
            break;
         }
      default:
         {
            EngSetLastError(ERROR_INVALID_PARAMETER);
            break;
         }
   }
   ObDereferenceObject(Process);
Exit:
   TRACE("Leave GetGuiResources, ret=%lu\n", Ret);
   UserLeave();
   return Ret;
}

BOOL WINAPI GetLastInputInfo(PLASTINPUTINFO plii)
{
    TRACE("%p\n", plii);

    if (plii->cbSize != sizeof (*plii))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    plii->dwTime = gpsi->dwLastRITEventTickCount;
    return TRUE;
}

BOOL WINAPI GetMonitorInfoW(
    IN HMONITOR hMonitor,
    OUT LPMONITORINFO pMonitorInfoUnsafe)
{
    PMONITOR pMonitor;
    MONITORINFOEXW MonitorInfo;
    NTSTATUS Status;
    BOOL bRet = FALSE;
    PWCHAR pwstrDeviceName;

    TRACE("Enter NtUserGetMonitorInfo\n");
    UserEnterShared();

    /* Get monitor object */
    pMonitor = UserGetMonitorObject(hMonitor);
    if (!pMonitor)
    {
        TRACE("Couldnt find monitor %p\n", hMonitor);
        goto cleanup;
    }

    /* Check if pMonitorInfoUnsafe is valid */
    if(pMonitorInfoUnsafe == NULL)
    {
        SetLastNtError(STATUS_INVALID_PARAMETER);
        goto cleanup;
    }

    pwstrDeviceName = ((PPDEVOBJ)(pMonitor->hDev))->pGraphicsDevice->szWinDeviceName;

    /* Get size of pMonitorInfoUnsafe */
    Status = MmCopyFromCaller(&MonitorInfo.cbSize, &pMonitorInfoUnsafe->cbSize, sizeof(MonitorInfo.cbSize));
    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        goto cleanup;
    }

    /* Check if size of struct is valid */
    if (MonitorInfo.cbSize != sizeof(MONITORINFO) &&
        MonitorInfo.cbSize != sizeof(MONITORINFOEXW))
    {
        SetLastNtError(STATUS_INVALID_PARAMETER);
        goto cleanup;
    }

    /* Fill monitor info */
    MonitorInfo.rcMonitor = pMonitor->rcMonitor;
    MonitorInfo.rcWork = pMonitor->rcWork;
    MonitorInfo.dwFlags = 0;
    if (pMonitor->IsPrimary)
        MonitorInfo.dwFlags |= MONITORINFOF_PRIMARY;

    /* Fill device name */
    if (MonitorInfo.cbSize == sizeof(MONITORINFOEXW))
    {
        RtlStringCbCopyNExW(MonitorInfo.szDevice,
                          sizeof(MonitorInfo.szDevice),
                          pwstrDeviceName,
                          (wcslen(pwstrDeviceName)+1) * sizeof(WCHAR),
                          NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
    }

    /* Output data */
    Status = MmCopyToCaller(pMonitorInfoUnsafe, &MonitorInfo, MonitorInfo.cbSize);
    if (!NT_SUCCESS(Status))
    {
        TRACE("GetMonitorInfo: MmCopyToCaller failed\n");
        SetLastNtError(Status);
        goto cleanup;
    }

    TRACE("GetMonitorInfo: success\n");
    bRet = TRUE;

cleanup:
    TRACE("Leave NtUserGetMonitorInfo, ret=%i\n", bRet);
    UserLeave();
    return bRet;
}
