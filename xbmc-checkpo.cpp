/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>

#if defined( WIN32 ) && defined( TUNE )
  #include <crtdbg.h>
  _CrtMemState startMemState;
  _CrtMemState endMemState;
#endif

#ifdef WINDOWS
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
 #endif


#include <stdio.h>
#include "tinyxml.h"
#include <string>
#include <map>
#include "ctime"
#include <algorithm>
#include "POUtils.h"

const std::string VERSION = "0.7";

#ifdef _MSC_VER
char DirSepChar = '\\';
  #include "dirent.h"
#else
char DirSepChar = '/';
  #include <dirent.h>
#endif

char* pSourceDirectory = NULL;
bool bCheckSourceLang;

std::string strHeader;
FILE * pPOTFile;
FILE * pLogFile;

std::map<uint32_t, CPOEntry> mapStrings;
typedef std::map<uint32_t, CPOEntry>::iterator itStrings;

std::vector<CPOEntry> vecClassicEntries;
typedef std::vector<CPOEntry>::iterator itClassicEntries;


void PrintUsage()
{
  printf
  (
  "Usage: xbmc-xml2po -s <sourcedirectoryname> (-p <projectname>) (-v <version>)\n"
  "parameter -s <name> for source root language directory, which contains the language dirs\n"
  "parameter -p <name> for project name, eg. xbmc.skin.confluence\n"
  "parameter -v <name> for project version, eg. FRODO_GIT251373f9c3\n\n"
  );
#ifdef _MSC_VER
  printf
  (
  "Note for Windows users: In case you have whitespace or any special character\n"
  "in any of the arguments, please use apostrophe around them. For example:\n"
  "xbmc-xml2po.exe -s \"C:\\xbmc dir\\language\" -p xbmc.core -v Frodo_GIT\n"
  );
#endif
return;
};

bool WriteMultilineComment(std::vector<std::string> vecCommnts, std::string prefix)
{
  if (!vecCommnts.empty())
  {
    for (size_t i=0; i < vecCommnts.size(); i++)
      fprintf(pPOTFile, "%s%s\n", prefix.c_str(), vecCommnts[i].c_str());
  }
  return !vecCommnts.empty();
}

bool WritePOFile(std::string strDir, std::string strLang)
{
  int writtenEntry = 0;
  std::string OutputPOFilename;
  std::string LCode = strHeader.substr(strHeader.find("Language: ")+10,2);
  bool bIsForeignLang = strLang != "English";

  OutputPOFilename = strDir + DirSepChar + strLang + DirSepChar + "strings.po.temp";

  // Initalize the output po document
  pPOTFile = fopen (OutputPOFilename.c_str(),"wb");
  if (pPOTFile == NULL)
  {
    printf("Error opening output file: %s\n", OutputPOFilename.c_str());
    return false;
  }
  printf("%s\t\t", LCode.c_str()); 

  fprintf(pPOTFile, "%s\n", strHeader.c_str());

  int previd = -1;
  bool bCommentWritten = false;
  for ( itStrings it = mapStrings.begin() ; it != mapStrings.end() ; it++)
  {
    int id = it->first;
    CPOEntry currEntry = it->second;

    bCommentWritten = false;
    if (!bIsForeignLang)
    {
      bCommentWritten = WriteMultilineComment(currEntry.interlineComm, "#");
      if (id-previd >= 2)
      {
        if (id-previd == 2 && previd > -1)
          fprintf(pPOTFile,"#empty string with id %i\n", id-1);
        if (id-previd > 2 && previd > -1)
          fprintf(pPOTFile,"#empty strings from id %i to %i\n", previd+1, id-1);
        bCommentWritten = true;
      }
      if (bCommentWritten) fprintf(pPOTFile, "\n");
    }

    if (!bIsForeignLang)
    {
      WriteMultilineComment(currEntry.translatorComm, "# ");
      WriteMultilineComment(currEntry.extractedComm, "#.");
      WriteMultilineComment(currEntry.referenceComm, "#:");
    }

    fprintf(pPOTFile,"msgctxt \"#%i\"\n", id);

    fprintf(pPOTFile,"msgid \"%s\"\n", currEntry.msgID.c_str());
    fprintf(pPOTFile,"msgstr \"%s\"\n\n", currEntry.msgStr.c_str());

    writtenEntry++;
    previd =id;
  }
  fclose(pPOTFile);

  printf("%i\t\t", writtenEntry);
  printf("%i\t\t", 0);
  printf("%s\n", OutputPOFilename.erase(0,strDir.length()).c_str());

  return true;
}


void ClearCPOEntry (CPOEntry &entry)
{
  entry.msgStrPlural.clear();
  entry.referenceComm.clear();
  entry.extractedComm.clear();
  entry.translatorComm.clear();
  entry.interlineComm.clear();
  entry.numID = 0;
  entry.msgID.clear();
  entry.msgStr.clear();
  entry.msgIDPlur.clear();
  entry.msgCtxt.clear();
  entry.Type = UNKNOWN_FOUND;
};

bool CheckPOFile(std::string strDir, std::string strLang)
{
  CPODocument PODoc;
  if (!PODoc.LoadFile(strDir + DirSepChar + strLang + DirSepChar + "strings.po"))
    return false;
  if (PODoc.GetEntryType() != HEADER_FOUND)
    printf ("POParser: No valid header found for this language");

  strHeader = PODoc.GetEntryData().Content.substr(1);

  int counter = 0;
  mapStrings.clear();
  vecClassicEntries.clear();

  bool bMultipleComment = false;
  std::vector<std::string> vecILCommnts;
  CPOEntry currEntry;
  int currType = UNKNOWN_FOUND;

  while ((PODoc.GetNextEntry()))
  {
    PODoc.ParseEntry();
    currEntry = PODoc.GetEntryData();
    currType = PODoc.GetEntryType();

    if (currType == COMMENT_ENTRY_FOUND)
    {
      if (!vecILCommnts.empty())
        bMultipleComment = true;
      vecILCommnts = currEntry.interlineComm;
      continue;
    }

    if (currType == ID_FOUND || currType == MSGID_FOUND || currType == MSGID_PLURAL_FOUND)
    {
      if (bMultipleComment)
        printf("POParser: multiple comment entries found. Using only the last one "
               "before the real entry. Entry after comments: %s", currEntry.Content.c_str());
      if (!currEntry.interlineComm.empty())
        printf("POParser: interline comments (eg. #comment) is not alowed inside "
               "a real po entry. Cleaned it. Problematic entry: %s", currEntry.Content.c_str());
      currEntry.interlineComm = vecILCommnts;
      bMultipleComment = false;
      vecILCommnts.clear();
      if (currType == ID_FOUND)
        mapStrings[currEntry.numID] = currEntry;
      else
        vecClassicEntries.push_back(currEntry);

      ClearCPOEntry(currEntry);
    }
  }

  WritePOFile(strDir, strLang);

  return true;
};

int main(int argc, char* argv[])
{
  // Basic syntax checking for "-x name" format
  while ((argc > 2) && (argv[1][0] == '-') && (argv[1][1] != '\x00') &&
    (argv[1][2] == '\x00'))
  {
    switch (argv[1][1])
    {
      case 's':
	if ((argv[2][0] == '\x00') || (argv[2][0] == '-'))
	  break;
        --argc; ++argv;
        pSourceDirectory = argv[1];
        break;
      case 'E':
        --argc; ++argv;
	bCheckSourceLang = true;
        break;
    }
    ++argv; --argc;
  }

  if (pSourceDirectory == NULL)
  {
    printf("\nWrong Arguments given !\n");
    char *path=NULL;
    size_t size;
    path=getcwd(path,size);
    return 1;
  }

  printf("\nXBMC-CHECKPO v%s by Team XBMC\n", VERSION);
  printf("\nResults:\n\n");
  printf("Langcode\tString match\tAuto contexts\tOutput file\n");
  printf("--------------------------------------------------------------\n");

  std::string WorkingDir(pSourceDirectory);
  if (WorkingDir[WorkingDir.length()-1] != DirSepChar)
    WorkingDir.append(&DirSepChar);

  pLogFile = fopen (WorkingDir.append("xbmc-checkpo.log").c_str(),"wb");
  if (pLogFile == NULL)
  {
    fclose(pLogFile);
    printf("Error opening logfile: %s\n", WorkingDir.append("xbmc-checkpo.log");
    return false;
  }
  fprintf(pLogFile, "XBMC.CHECKPO v%s started", VERSION);

  DIR* Dir;
  struct dirent *DirEntry;
  Dir = opendir(WorkingDir.c_str());
  int langcounter =0;

  while((DirEntry=readdir(Dir)))
  {
    std::string LName = "";
    if (DirEntry->d_type == DT_DIR && DirEntry->d_name[0] != '.')
    {
      if (CheckPOFile(WorkingDir, DirEntry->d_name))
      {
        std::string pofilename = WorkingDir + DirSepChar + DirEntry->d_name + DirSepChar + "strings.po";
        std::string bakpofilename = WorkingDir + DirSepChar + DirEntry->d_name + DirSepChar + "strings.po.bak";
        std::string temppofilename = WorkingDir + DirSepChar + DirEntry->d_name + DirSepChar + "strings.po.temp";

        rename(pofilename.c_str(), bakpofilename.c_str());
        rename(temppofilename.c_str(),pofilename.c_str());
        langcounter++;
      }
    }
  }

  printf("\nReady. Processed %i languages.\n", langcounter+1);

//  if (bUnknownLangFound)
//    printf("\nWarning: At least one language found with unpaired language code !\n"
//      "Please edit the .po file manually and correct the language code, plurals !\n"
//      "Also please report this to alanwww1@xbmc.org if possible !\n\n");
  return 0;
}
