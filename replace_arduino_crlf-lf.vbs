'IDir = "D:\Electric\HeatPump\Control"
Find = vbCrLf ' 13, 10. строка поиска
Repl = vbLf   ' 10.     строка замены
Recurse = 0   ' 1 - обрабатывать всю структуру
 
Set FSO = CreateObject("Scripting.FileSystemObject")
Set Shell = CreateObject("Shell.Application")
IDir = FSO.GetAbsolutePathName(".")
ForFolder IDir
 
Sub ForFolder(Folder)
    Set Items = Shell.NameSpace(Folder).Items
    Items.Filter 73920, "*.h;*.ino"
    For Each i In Items
        With FSO.OpenTextFile(i.Path)
            All = .ReadAll
            .Close
        End With
        With FSO.OpenTextFile(i.Path, 2)
            .Write Replace(All, Find, Repl)
            .Close
        End With
    Next
    If Recurse = 1 Then
        For Each F In FSO.GetFolder(Folder).SubFolders
            ForFolder F.Path
        Next
    End If
End Sub