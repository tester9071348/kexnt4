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
