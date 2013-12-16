#pragma once

bool PspSaveData_Update();
void PspSaveData_Save(const char* appName, const char* saveName, const char* saveDesc, const char* saveDescLong, const void* bytes, int byteCount, bool allowAsync);
bool PspSaveData_Load(const char* appName, const char* saveName, void* bytes, int byteCount, int& o_byteCount);
void PspSaveData_Finish(bool warn);
bool PspSaveData_IsSaveInProgress();
void PspSaveData_Remove(const char* appName, const char* saveName);
bool PspSaveData_Exists(const char* appName, const char* saveName);
