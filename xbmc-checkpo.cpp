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
 */#include "utils/TimeUtils.h"
 

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

#ifdef _MSC_VER
char DirSepChar = '\\';
  #include "dirent.h"
#else
char DirSepChar = '/';
  #include <dirent.h>
#endif

char* pSourceDirectory = NULL;
bool bCheckSourceLang;

std::map<uint32_t, std::vector<CPOEntry>> mapStrings;
typedef std::map<uint32_t, std::vector<CPOEntry>>::iterator itStrings;

std::vector<std::vector<CPOEntry>> vecClassicEntries;
typedef std::vector<std::vector<CPOEntry>>::iterator itClassicEntries;


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
}

bool CheckPOFile(std::string strDir, std::string strLang)
{
  CPODocument PODoc;
  if (!PODoc.LoadFile(strDir + DirSepChar + strLang + DirSepChar + "strings.po"))
    return false;

  int counter = 0;

  while ((PODoc.GetNextEntry()))
  {
    printf("Type:%i\n", PODoc.GetEntryType());
    PODoc.ParseEntry();
    CPOEntry POEntry = PODoc.GetEntryData();
    printf("id:%i\n", POEntry.numID);
    printf("msgid:%s\n", POEntry.msgID.c_str());
    printf("msgstr:%s\n", POEntry.msgStr.c_str());
    printf("msgctxt:%s\n", POEntry.msgCtxt.c_str());
    if (!POEntry.translComments.empty())
      printf("commenttran:%s\n", POEntry.translComments[0].c_str());
    if (!POEntry.occurComments.empty())
      printf("commentocc:%s\n", POEntry.occurComments[0].c_str());
    if (!POEntry.otherComments.empty())
    {
     for (size_t i=0;i<POEntry.otherComments.size();i++) printf("commentother:%s\n", POEntry.otherComments[i].c_str());
    }
    printf("\n\n\n");
  }

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

  printf("\nXBMC-CHECKPO v0.7 by Team XBMC\n");
  printf("\nResults:\n\n");
  printf("Langcode\tString match\tAuto contexts\tOutput file\n");
  printf("--------------------------------------------------------------\n");

  std::string WorkingDir(pSourceDirectory);
  if (WorkingDir[WorkingDir.length()-1] != DirSepChar)
    WorkingDir.append(&DirSepChar);

//  if (bCheckSourceLang)
//  {
    if (!CheckPOFile(WorkingDir, "English"))
    {
      printf("Fatal error: no English source xml file found or it is corrupted\n");
      return 1;
    }
    else
      return 0;
//  }
/*
  ConvertXML2PO(WorkingDir + "English" + DirSepChar, "en", 2, "(n != 1)", false);

  DIR* Dir;
  struct dirent *DirEntry;
  Dir = opendir(WorkingDir.c_str());
  int langcounter =0;

  while((DirEntry=readdir(Dir)))
  {
    std::string LName = "";
    if (DirEntry->d_type == DT_DIR && DirEntry->d_name[0] != '.' && strcmp(DirEntry->d_name, "English"))
    {
      if (loadXMLFile(xmlDocForeignInput, WorkingDir + DirEntry->d_name + DirSepChar + "strings.xml",
          &mapForeignXmlId, false))
      {
        ConvertXML2PO(WorkingDir + DirEntry->d_name + DirSepChar, FindLangCode(DirEntry->d_name),
                      GetnPlurals(DirEntry->d_name), GetPlurForm(DirEntry->d_name), true);
        langcounter++;
      }
    }
  }

  printf("\nReady. Processed %i languages.\n", langcounter+1);

  if (bUnknownLangFound)
    printf("\nWarning: At least one language found with unpaired language code !\n"
      "Please edit the .po file manually and correct the language code, plurals !\n"
      "Also please report this to alanwww1@xbmc.org if possible !\n\n");
  return 0; */
}
