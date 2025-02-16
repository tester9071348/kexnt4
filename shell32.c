HRESULT WINAPI SHGetFolderPathW(
	HWND hwndOwner,    /* [I] owner window */
	int nFolder,       /* [I] CSIDL identifying the folder */
	HANDLE hToken,     /* [I] access token */
	DWORD dwFlags,     /* [I] which path to return */
	LPWSTR pszPath)    /* [O] converted path */
{
    HRESULT hr =  SHGetFolderPathAndSubDirW(hwndOwner, nFolder, hToken, dwFlags, NULL, pszPath);
    if(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    return hr;
}

BOOL WINAPI SHGetSpecialFolderPathW (
	HWND hwndOwner,
	LPWSTR szPath,
	int nFolder,
	BOOL bCreate)
{
    return SHGetFolderPathW(hwndOwner, nFolder + (bCreate ? CSIDL_FLAG_CREATE : 0), NULL, 0,
                            szPath) == S_OK;
}
