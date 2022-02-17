#include "windows.h"
#include <stdio.h>
#include <iostream>

using namespace std;

void volumeInfo(WCHAR *volName)
{
  {
    //First some basic volume info
    WCHAR volumeName[MAX_PATH + 1] = { 0 };
    WCHAR fileSystemName[MAX_PATH + 1] = { 0 };
    DWORD serialNumber = 0;
    DWORD maxComponentLen = 0;
    DWORD fileSystemFlags = 0;
    if (GetVolumeInformation(volName, volumeName, ARRAYSIZE(volumeName), &serialNumber, &maxComponentLen, &fileSystemFlags, fileSystemName, ARRAYSIZE(fileSystemName)))
    {
      wprintf(L"Label: [%s]  ", volumeName);
      wprintf(L"SerNo: %lu  ", serialNumber);
      wprintf(L"FS: [%s]\n", fileSystemName);
  //    wprintf(L"Max Component Length: %lu\n", maxComponentLen);
    }
    else
    {
      TCHAR msg[MAX_PATH + 1];
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), msg, MAX_PATH, NULL);
      wprintf(L"Last error: %s", msg);
    }
  }
  {
    //The following code finds all folders that are mount points on this volume (empty folder that has another volume mounted-in)
    //This requires administrative privileges so unless you run the app as an admin, the function will simply return nothing
    //It's pretty much useless anyway because the same info can be obtained in the following section where we get mount points for a volume - so reverse lookup is quite possible
    HANDLE mp;
    WCHAR volumeName[MAX_PATH + 1] = { 0 };
    bool success;
    mp = FindFirstVolumeMountPoint(volName, volumeName, MAX_PATH);
    success = mp != INVALID_HANDLE_VALUE;
    if (!success)
    { //This will yield "Access denied" unless we run the app in administrative mode
      TCHAR msg[MAX_PATH + 1];
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), msg, MAX_PATH, NULL);
      wprintf(L"Evaluate mount points error: %s", msg);
    }
    while (success)
    {
      wcout << L"Mount point: " << volumeName << endl;
      success = FindNextVolumeMountPoint(mp, volumeName, MAX_PATH) != 0;
    }
    FindVolumeMountPointClose(mp);
  }

  {
    //Now find the mount points for this volume
    DWORD charCount = MAX_PATH;
    WCHAR *mp = NULL, *mps = NULL;
    bool success;

    while (true)
    {
      mps = new WCHAR[charCount];
      success = GetVolumePathNamesForVolumeNameW(volName, mps, charCount, &charCount) != 0;
      if (success || GetLastError() != ERROR_MORE_DATA) 
        break;
      delete [] mps;
      mps = NULL;
    }
    if (success)
    {
      for (mp = mps; mp[0] != '\0'; mp += wcslen(mp))
        wcout << L"Mount point: " << mp << endl;
    }
    delete [] mps;
  }

  {
    //And the type of this volume
    switch (GetDriveType(volName))
    {
    case DRIVE_UNKNOWN:     wcout << "unknown"; break;
    case DRIVE_NO_ROOT_DIR: wcout << "bad drive path"; break;
    case DRIVE_REMOVABLE:   wcout << "removable"; break;
    case DRIVE_FIXED:       wcout << "fixed"; break;
    case DRIVE_REMOTE:      wcout << "remote"; break;
    case DRIVE_CDROM:       wcout << "CD ROM"; break;
    case DRIVE_RAMDISK:     wcout << "RAM disk"; break;
    }
    wcout << endl;
  }
  {
    //This part of code will determine what this volume is composed of. The returned disk extents are actual disk partitions
    HANDLE volH;
    bool success;
    PVOLUME_DISK_EXTENTS vde;
    DWORD bret;

    volName[wcslen(volName) - 1] = '\0';
    volH = CreateFile(volName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (volH == INVALID_HANDLE_VALUE)
    {
      TCHAR msg[MAX_PATH + 1];
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), msg, MAX_PATH, NULL);
      wprintf(L"Open volume error: %s", msg);
      return;
    }
    bret = sizeof(VOLUME_DISK_EXTENTS) + 256 * sizeof(DISK_EXTENT);
    vde = (PVOLUME_DISK_EXTENTS)malloc(bret);
    success = DeviceIoControl(volH, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, (void *)vde, bret, &bret, NULL) != 0;
    if (!success)
      return;
    for (unsigned i = 0; i < vde->NumberOfDiskExtents; i++)
      wcout << L"Volume extent: " << vde->Extents[i].DiskNumber << L" "<< vde->Extents[i].StartingOffset.QuadPart << L" - " << vde->Extents[i].ExtentLength.QuadPart << endl;
    free(vde);
    CloseHandle(volH);
  }
}

bool findVolume(WCHAR *volName, int diskno, long long offs, long long len)
{
  HANDLE vol;
  bool success;

  vol = FindFirstVolume(volName, MAX_PATH); //I'm cheating here! I only know volName is MAX_PATH long because I wrote so in enumPartitions findVolume call
  success = vol != INVALID_HANDLE_VALUE;
  while (success)
  {
    //We are now enumerating volumes. In order for this function to work, we need to get partitions that compose this volume
    HANDLE volH;
    PVOLUME_DISK_EXTENTS vde;
    DWORD bret;

    volName[wcslen(volName) - 1] = '\0'; //For this CreateFile, volume must be without trailing backslash
    volH = CreateFile(volName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    volName[wcslen(volName)] = '\\';
    if (volH != INVALID_HANDLE_VALUE)
    {
      bret = sizeof(VOLUME_DISK_EXTENTS) + 256 * sizeof(DISK_EXTENT);
      vde = (PVOLUME_DISK_EXTENTS)malloc(bret);
      if (DeviceIoControl(volH, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, (void *)vde, bret, &bret, NULL))
      {
        for (unsigned i = 0; i < vde->NumberOfDiskExtents; i++)
          if (vde->Extents[i].DiskNumber == diskno &&
              vde->Extents[i].StartingOffset.QuadPart == offs &&
              vde->Extents[i].ExtentLength.QuadPart == len)
          {
            free(vde);
            CloseHandle(volH);
            FindVolumeClose(vol);
            return true;
          }
      }
      free(vde);
      CloseHandle(volH);
    }

    success = FindNextVolume(vol, volName, MAX_PATH) != 0;
  }
  FindVolumeClose(vol);
  return false;
}

void enumPartitions()
{
  GUID PARTITION_BASIC_DATA_GUID; //This one is actually defined in windows DDK headers, but the linker kills me when I try to use it in a normal (non driver) program
  PARTITION_BASIC_DATA_GUID.Data1 = 0xEBD0A0A2L;
  PARTITION_BASIC_DATA_GUID.Data2 = 0xB9E5;
  PARTITION_BASIC_DATA_GUID.Data3 = 0x4433;
  PARTITION_BASIC_DATA_GUID.Data4[0] = 0x87;
  PARTITION_BASIC_DATA_GUID.Data4[1] = 0xC0;
  PARTITION_BASIC_DATA_GUID.Data4[2] = 0x68;
  PARTITION_BASIC_DATA_GUID.Data4[3] = 0xB6;
  PARTITION_BASIC_DATA_GUID.Data4[4] = 0xB7;
  PARTITION_BASIC_DATA_GUID.Data4[5] = 0x26;
  PARTITION_BASIC_DATA_GUID.Data4[6] = 0x99;
  PARTITION_BASIC_DATA_GUID.Data4[7] = 0xC7;

  //These structures are needed for IOCTL_DISK_GET_DRIVE_LAYOUT_EX below, but we allocate here so that we don't have to worry about freeing until end of function
  PDRIVE_LAYOUT_INFORMATION_EX partitions;
  DWORD partitionsSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + 127 * sizeof(PARTITION_INFORMATION_EX);
  partitions = (PDRIVE_LAYOUT_INFORMATION_EX)malloc(partitionsSize);

  for (int i = 0; ; i++)
  { //The main drive loop. This loop enumerates all physical drives windows currently sees
    //Note that empty removable drives are not enumerated with this method
    //Only volume enumeration will allow you to see those
    WCHAR volume[MAX_PATH];
    wsprintf(volume, L"\\\\.\\PhysicalDrive%d", i);

    //We are enumerating registered physical drives here. So the following CreateFile verifies if this disk even exists. If it does not, we have finished our enumeration
    //We also need the handle to get further info on the storage device
    HANDLE h = CreateFile(volume, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    bool success = h != INVALID_HANDLE_VALUE;
    if (!success)
      break;

    wcout << endl << endl << endl << L"Disk #" << i << endl;
    //Get information on storage device itself
    DISK_GEOMETRY_EX driveGeom;
    DWORD ior;
    success = DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &driveGeom, sizeof(driveGeom), &ior, NULL) != 0;
    if (success)
      wcout << L"Size: " << driveGeom.DiskSize.QuadPart << L"   " << driveGeom.DiskSize.QuadPart / 1024 / 1024 / 1024 << L" GB" << endl; //We could take other info from structure, but let's say disk size is enough
    //Get info on partitions
    success = DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, partitions, partitionsSize, &ior, NULL) != 0;
    if (success)
    {
      switch (partitions->PartitionStyle)
      {
      case PARTITION_STYLE_MBR: wcout << L"Partition type " << L"MBR" << endl; break;
      case PARTITION_STYLE_GPT: wcout << L"Partition type " << L"GPT" << endl; break;
      default: wcout << L"Partition type " << L"unknown" << endl; break;
      }

      for (int iPart = 0; iPart < int(partitions->PartitionCount); iPart++)
      { //Loop through partition records that we found
        bool partGood = false;
        if (partitions->PartitionEntry[iPart].PartitionStyle == PARTITION_STYLE_MBR && partitions->PartitionEntry[iPart].Mbr.PartitionType != PARTITION_ENTRY_UNUSED && partitions->PartitionEntry[iPart].Mbr.RecognizedPartition)
        { //For MBR partitions, the partition record must not be unused. That way we know it's a valid partition
          wcout << endl << endl << L"Partition " << iPart + 1 << L" offset: " << partitions->PartitionEntry[iPart].StartingOffset.QuadPart << L" length: " << partitions->PartitionEntry[iPart].PartitionLength.QuadPart << L"  " << partitions->PartitionEntry[iPart].PartitionLength.QuadPart / 1024 / 1024 << " MB" << endl;
          partGood = true;
        }
        else if (partitions->PartitionEntry[iPart].PartitionStyle == PARTITION_STYLE_GPT && partitions->PartitionEntry[iPart].Gpt.PartitionType == PARTITION_BASIC_DATA_GUID)
        { //For GPT partitions, partition type must be PARTITION_BASIC_DATA_GUID for it to be usable
          //Quite frankly, windows is doing some shady stuff here: for each GPT disk, windows takes some 128MB partition for its own use. 
          //I have no idea what that partition is used for, but it seems like an utter waste of space. My first disk was 10MB for crist's sake :(
          wcout << endl << endl << L"Partition " << iPart + 1 << L" offset: " << partitions->PartitionEntry[iPart].StartingOffset.QuadPart << L" length: " << partitions->PartitionEntry[iPart].PartitionLength.QuadPart << L"  " << partitions->PartitionEntry[iPart].PartitionLength.QuadPart / 1024 / 1024 << " MB" << endl;
          partGood = true;
        }
        if (partGood == true)
        {
          WCHAR volume[MAX_PATH];
          if (findVolume(volume, i, partitions->PartitionEntry[iPart].StartingOffset.QuadPart, partitions->PartitionEntry[iPart].PartitionLength.QuadPart))
          {
            wcout << endl << volume <<endl;
            volumeInfo(volume);
          }
        }
      }
    }
    CloseHandle(h);
  }
  free(partitions);
}

int main(int argc, char* argv[])
{
  enumPartitions();
  bool success; //Just so that the app waits before terminating
  cin >> success;
  return 0;
}
