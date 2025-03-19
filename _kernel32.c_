DWORD WINAPI GetLongPathNameW(IN LPCWSTR lpszShortPath,
                 OUT LPWSTR lpszLongPath,
                 IN DWORD cchBuffer)
{
    PWCHAR Path, Original, First, Last, Buffer, Src, Dst;
    SIZE_T Length, ReturnLength;
    WCHAR LastChar;
    HANDLE FindHandle;
    ULONG ErrorMode;
    BOOLEAN Found = FALSE;
    WIN32_FIND_DATAW FindFileData;

    /* Initialize so Quickie knows there's nothing to do */
    Buffer = Original = NULL;
    ReturnLength = 0;

    /* First check if the input path was obviously NULL */
    if (!lpszShortPath)
    {
        /* Fail the request */
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* We will be touching removed, removable drives -- don't warn the user */
    ErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

    /* Do a simple check to see if the path exists */
    if (GetFileAttributesW(lpszShortPath) == INVALID_FILE_ATTRIBUTES)
    {
        /* It doesn't, so fail */
        ReturnLength = 0;
        goto Quickie;
    }

    /* Now get a pointer to the actual path, skipping indicators */
    Path = SkipPathTypeIndicator_U((LPWSTR)lpszShortPath);

    /* Is there any path or filename in there? */
    if (!(Path) ||
        (*Path == UNICODE_NULL) ||
        !(FindLFNorSFN_U(Path, &First, &Last, FALSE)))
    {
        /* There isn't, so the long path is simply the short path */
        ReturnLength = wcslen(lpszShortPath);

        /* Is there space for it? */
        if ((cchBuffer > ReturnLength) && (lpszLongPath))
        {
            /* Make sure the pointers aren't already the same */
            if (lpszLongPath != lpszShortPath)
            {
                /* They're not -- copy the short path into the long path */
                RtlMoveMemory(lpszLongPath,
                              lpszShortPath,
                              ReturnLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));
            }
        }
        else
        {
            /* Otherwise, let caller know we need a bigger buffer, include NULL */
            ReturnLength++;
        }
        goto Quickie;
    }

    /* We are still in the game -- compute the current size */
    Length = wcslen(lpszShortPath) + sizeof(ANSI_NULL);
    Original = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length * sizeof(WCHAR));
    if (!Original) goto ErrorQuickie;

    /* Make a copy of it */
    RtlMoveMemory(Original, lpszShortPath, Length * sizeof(WCHAR));

    /* Compute the new first and last markers */
    First = &Original[First - lpszShortPath];
    Last = &Original[Last - lpszShortPath];

    /* Set the current destination pointer for a copy */
    Dst = lpszLongPath;

    /*
     * Windows allows the paths to overlap -- we have to be careful with this and
     * see if it's same to do so, and if not, allocate our own internal buffer
     * that we'll return at the end.
     *
     * This is also why we use RtlMoveMemory everywhere. Don't use RtlCopyMemory!
     */
    if ((cchBuffer) && (lpszLongPath) &&
        (((lpszLongPath >= lpszShortPath) && (lpszLongPath < &lpszShortPath[Length])) ||
         ((lpszLongPath < lpszShortPath) && (&lpszLongPath[cchBuffer] >= lpszShortPath))))
    {
        Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, cchBuffer * sizeof(WCHAR));
        if (!Buffer) goto ErrorQuickie;

        /* New destination */
        Dst = Buffer;
    }

    /* Prepare for the loop */
    Src = Original;
    ReturnLength = 0;
    while (TRUE)
    {
        /* Current delta in the loop */
        Length = First - Src;

        /* Update the return length by it */
        ReturnLength += Length;

        /* Is there a delta? If so, is there space and buffer for it? */
        if ((Length) && (cchBuffer > ReturnLength) && (lpszLongPath))
        {
            RtlMoveMemory(Dst, Src, Length * sizeof(WCHAR));
            Dst += Length;
        }

        /* "Terminate" this portion of the path's substring so we can do a find */
        LastChar = *Last;
        *Last = UNICODE_NULL;
        FindHandle = FindFirstFileW(Original, &FindFileData);
        *Last = LastChar;

        /* This portion wasn't found, so fail */
        if (FindHandle == INVALID_HANDLE_VALUE)
        {
            ReturnLength = 0;
            break;
        }

        /* Close the find handle */
        FindClose(FindHandle);

        /* Now check the length of the long name */
        Length = wcslen(FindFileData.cFileName);
        if (Length)
        {
            /* This is our new first marker */
            First = FindFileData.cFileName;
        }
        else
        {
            /* Otherwise, the name is the delta between our current markers */
            Length = Last - First;
        }

        /* Update the return length with the short name length, if any */
        ReturnLength += Length;

        /* Once again check for appropriate space and buffer */
        if ((cchBuffer > ReturnLength) && (lpszLongPath))
        {
            /* And do the copy if there is */
            RtlMoveMemory(Dst, First, Length * sizeof(WCHAR));
            Dst += Length;
        }

        /* Now update the source pointer */
        Src = Last;
        if (*Src == UNICODE_NULL) break;

        /* Are there more names in there? */
        Found = FindLFNorSFN_U(Src, &First, &Last, FALSE);
        if (!Found) break;
    }

    /* The loop is done, is there anything left? */
    if (ReturnLength)
    {
        /* Get the length of the straggling path */
        Length = wcslen(Src);
        ReturnLength += Length;

        /* Once again check for appropriate space and buffer */
        if ((cchBuffer > ReturnLength) && (lpszLongPath))
        {
            /* And do the copy if there is -- accounting for NULL here */
            RtlMoveMemory(Dst, Src, Length * sizeof(WCHAR) + sizeof(UNICODE_NULL));

            /* What about our buffer? */
            if (Buffer)
            {
                /* Copy it into the caller's long path */
                RtlMoveMemory(lpszLongPath,
                              Buffer,
                              ReturnLength * sizeof(WCHAR)  + sizeof(UNICODE_NULL));
            }
        }
        else
        {
            /* Buffer is too small, let the caller know, making space for NULL */
            ReturnLength++;
        }
    }

    /* We're all done */
    goto Quickie;

ErrorQuickie:
    /* This is the goto for memory failures */
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);

Quickie:
    /* General function end: free memory, restore error mode, return length */
    if (Original) RtlFreeHeap(RtlGetProcessHeap(), 0, Original);
    if (Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    SetErrorMode(ErrorMode);
    return ReturnLength;
}
