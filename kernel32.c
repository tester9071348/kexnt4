static void GetLongPathNameW(void)
{
    DWORD length, expanded;
    BOOL ret;
    HANDLE file;
    WCHAR empty[MAX_PATH];
    WCHAR tempdir[MAX_PATH], name[200];
    WCHAR dirpath[4 + MAX_PATH + 200]; /* To ease removal */
    WCHAR shortpath[4 + MAX_PATH + 200 + 1 + 200];
    static const WCHAR prefix[] = { '\\','\\','?','\\', 0};
    static const WCHAR backslash[] = { '\\', 0};
    static const WCHAR letterX[] = { 'X', 0};

    SetLastError(0xdeadbeef); 
    length = GetLongPathNameW(NULL,NULL,0);
    ok(0==length,"GetLongPathNameW returned %ld but expected 0\n",length);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"GetLastError returned %ld but expected ERROR_INVALID_PARAMETER\n",GetLastError());

    SetLastError(0xdeadbeef); 
    empty[0]=0;
    length = GetLongPathNameW(empty,NULL,0);
    ok(0==length,"GetLongPathNameW returned %ld but expected 0\n",length);
    ok(GetLastError()==ERROR_PATH_NOT_FOUND,"GetLastError returned %ld but expected ERROR_PATH_NOT_FOUND\n",GetLastError());

    /* Create a long path name. The path needs to exist for these tests to
     * succeed so we need the "\\?\" prefix when creating directories and
     * files.
     */
    name[0] = 0;
    while (lstrlenW(name) < (ARRAY_SIZE(name) - 1))
        lstrcatW(name, letterX);

    GetTempPathW(MAX_PATH, tempdir);

    lstrcpyW(shortpath, prefix);
    lstrcatW(shortpath, tempdir);
    lstrcatW(shortpath, name);
    lstrcpyW(dirpath, shortpath);
    ret = CreateDirectoryW(shortpath, NULL);
    ok(ret, "Could not create the temporary directory : %ld\n", GetLastError());
    lstrcatW(shortpath, backslash);
    lstrcatW(shortpath, name);

    /* Path does not exist yet and we know it overruns MAX_PATH */

    /* No prefix */
    SetLastError(0xdeadbeef);
    length = GetLongPathNameW(shortpath + 4, NULL, 0);
    ok(length == 0, "Expected 0, got %ld\n", length);
    todo_wine
    ok(GetLastError() == ERROR_PATH_NOT_FOUND,
       "Expected ERROR_PATH_NOT_FOUND, got %ld\n", GetLastError());
    /* With prefix */
    SetLastError(0xdeadbeef);
    length = GetLongPathNameW(shortpath, NULL, 0);
    todo_wine
    {
    ok(length == 0, "Expected 0, got %ld\n", length);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_PATH_NOT_FOUND, got %ld\n", GetLastError());
    }

    file = CreateFileW(shortpath, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE,
       "Could not create the temporary file : %ld.\n", GetLastError());
    CloseHandle(file);

    /* Path exists */

    /* No prefix */
    SetLastError(0xdeadbeef);
    length = GetLongPathNameW(shortpath + 4, NULL, 0);
    todo_wine
    {
    ok(length == 0, "Expected 0, got %ld\n", length);
    ok(GetLastError() == ERROR_PATH_NOT_FOUND, "Expected ERROR_PATH_NOT_FOUND, got %ld\n", GetLastError());
    }
    /* With prefix */
    expanded = 4 + (GetLongPathNameW(tempdir, NULL, 0) - 1) + lstrlenW(name) + 1 + lstrlenW(name) + 1;
    SetLastError(0xdeadbeef);
    length = GetLongPathNameW(shortpath, NULL, 0);
    ok(length == expanded, "Expected %ld, got %ld\n", expanded, length);

    /* NULL buffer with length crashes on Windows */
    if (0)
        GetLongPathNameW(shortpath, NULL, 20);

    ok(DeleteFileW(shortpath), "Could not delete temporary file\n");
    ok(RemoveDirectoryW(dirpath), "Could not delete temporary directory\n");
}
