# scope-handle
scoped handle for windows objects
### [scope17 from PeterSommerlad](https://github.com/PeterSommerlad/scope17)
Example:  
```
int main()
{
  const TCHAR* strName = _T("data\\create_file.txt");
  auto unr = make_unique_resource_checked(CreateFile(strName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL),
            INVALID_HANDLE_VALUE,
            CloseHandleDeleter{});
  if (INVALID_HANDLE_VALUE != ur.get()) {
    // do something
  }
}
```
