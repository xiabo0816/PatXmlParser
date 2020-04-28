// XmlParser.cpp: 定义应用程序的入口点。
//

#include "XmlParser.h"
#include "../pugixml/pugixml.hpp"
#include "../inifile2/inifile.h"
#include <set>
#include <map>
#include <algorithm>
//#include <time.h>

#define MAX_PATH		260
#define MAX_WORD_LEN	1024 * 2
#define MAX_SENT_LEN	1024 * 100
#define EXTRA_INFO_SEPARATOR	';'
#define EXTRA_INFO_MAXNUM		256

#ifdef _WIN32
#include "stdafx.h"
#include <direct.h>  
#include <atlbase.h>
#define mkdir(a) mkdir((a))  
#define I64dlld "I64d"
#endif

#ifndef _WIN32
#include <unistd.h>
#include <math.h>
#include <sys/types.h> 
#include <sys/stat.h>
#define mkdir(a) mkdir((a),0777)
#define I64dlld "lld"
#endif

using namespace inifile;
using namespace pugi;
using namespace std;
map<string, vector<string>> g_mExtraInfo;
/******************************************************************************
 *
 * thread support
 *
 *****************************************************************************/
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct thread_t thread_t;
	extern thread_t* thread_create_ex(void* lpStartAddress, void* lpParameter);
	extern void thread_join_ex(thread_t* t, long long dwMilliseconds);
	typedef struct mutex_t mutex_t;
	extern mutex_t* mutex_create(void);
	extern void mutex_destroy(mutex_t*);
	extern void mutex_secure(mutex_t*);
	extern void mutex_release(mutex_t*);
	typedef void thread_proc_t(void* arg);
#ifdef __cplusplus
}
#endif

bool ReadExtraInfo(char* psPath) {

// 	g_mExtraInfo
	FILE* fpInp = fopen(psPath, "rb");
	if (fpInp == NULL)
		return 0;

	char szLine[MAX_SENT_LEN];
	char* psTmp;
	while (fgets(szLine, MAX_SENT_LEN, fpInp) != NULL) {
		if (szLine[strlen(szLine) - 1] == '\n') {
			szLine[strlen(szLine) - 1] = 0;
		}
		if (szLine[strlen(szLine) - 1] == '\r') {
			szLine[strlen(szLine) - 1] = 0;
		}
		psTmp = strchr(szLine, EXTRA_INFO_SEPARATOR);
		if (!psTmp) continue;
		*psTmp = 0; psTmp++;
		if (psTmp) for (size_t i = 0; i < strlen(psTmp); i++)
		{
			if (psTmp[i] == ';') psTmp[i] = '-';
			if (psTmp[i] == '|') psTmp[i] = '_';
		}
		if (psTmp)
			g_mExtraInfo[szLine].push_back(psTmp);

	}
	fclose(fpInp);

	return true;
}

void PrintUsage(char* psCmd)
{
	printf("Usage:\n");
	printf("%s config.ini list.txt outpath ExtraInfo\n", psCmd);
}

class CConfig
{
public:
	bool ConfigInit(char* psConfig) {
		IniFile ini;
		if (ini.Load(psConfig) == ERR_OPEN_FILE_FAILED) return false;
		for (vector<IniItem>::iterator it = ini.getSection("FIELD")->begin(); it != ini.getSection("FIELD")->end(); it++) {
			m_mField[(*it).value] = (*it).key;
		}
		for (vector<IniItem>::iterator it = ini.getSection("INFO")->begin(); it != ini.getSection("INFO")->end(); it++) {
			m_mInfo[(*it).value] = (*it).key;
		}
		for (vector<IniItem>::iterator it = ini.getSection("EXTRAINFO")->begin(); it != ini.getSection("EXTRAINFO")->end(); it++) {
			m_mExInfo[(*it).value] = (*it).key;
		}
		return true;
	}
	void ConfigExit() {

	};
public:
	map<string, string> m_mField;
	map<string, string> m_mInfo;
	map<string, string> m_mExInfo;
	CConfig() {
	};

};


class CThreadInfo
{
public:
	unsigned m_nID;
	vector<string> m_vFileList;
	string  m_strOutPath;
	map<string, string> m_mField;
	map<string, string> m_mInfo;
	map<string, string> m_mExInfo;
public:
	CThreadInfo() {
		m_nID = 0;
	};

};

struct simple_walker:xml_tree_walker
{
	string m_strRes;
	virtual bool for_each(xml_node& node)
	{
		m_strRes += node.value();
		return true;
	}

	string get() {
		return m_strRes;
	}
};

bool isSpace(char x) { return x == ' '; }
bool _cleaner(string t, char* buf) {
	t.erase(std::remove_if(t.begin(), t.end(), isSpace), t.end());
	string res = t;
	for (size_t i = 0; i < res.size(); i++)
	{
		if (res.c_str()[i] == ';') {
			buf[i] = '|';
		}
		else {
			buf[i] = res.c_str()[i];
		}
	}
	return true;
}
void ThreadWorker(void* objInp)
{
	CThreadInfo* pThreadInfo;
	pThreadInfo = (CThreadInfo*)objInp;
	int nLine = 0;
	FILE* fpOut = NULL;
	char szFile[MAX_PATH];
	sprintf(szFile, "%s_%d.dat", pThreadInfo->m_strOutPath.c_str(), pThreadInfo->m_nID);

	if ((fpOut = fopen(szFile, "wb")) == NULL) {
		printf("Error open %s\n", szFile);
		return;
	}

	for (unsigned int i = 0; i < pThreadInfo->m_vFileList.size(); i++) {
		if (i % 100 == 0) {
			printf("Thread %d, file %d\r", pThreadInfo->m_nID, nLine);
		}

		xml_document doc;
		xml_parse_result result = doc.load_file(pThreadInfo->m_vFileList[i].c_str());
		if (result.status != status_ok)
			cout << "Load [" << pThreadInfo->m_vFileList[i].c_str()<< "]: " << result.description() << endl;
		
		fprintf(fpOut, "<#>0 ");
		for (map<string, string>::iterator it = pThreadInfo->m_mInfo.begin(); it != pThreadInfo->m_mInfo.end(); it++)
		{
			xpath_node_set tools = doc.select_nodes((*it).first.c_str());
			if (tools.size() == 0)
				cout << "Load [" << pThreadInfo->m_vFileList[i].c_str() << "] Info NULL: " << (*it).second.c_str() << endl;

			fprintf(fpOut, "%s=", (*it).second.c_str());
			for (pugi::xpath_node_set::const_iterator itr = tools.begin(); itr != tools.end(); ++itr)
			{
				// std::cout << (*itr).node().child_value() << "\n";
				simple_walker walker;
				(*itr).node().traverse(walker);
				string t = walker.get();
				char buffer[MAX_SENT_LEN];
				memset(buffer, 0, MAX_SENT_LEN);
				_cleaner(t, buffer);
				fprintf(fpOut, "%s", buffer);
				if ((*it).first.find("|") == -1 && itr+1 != tools.end()) fprintf(fpOut, "|");
			}
			fprintf(fpOut, ";");
		}
		char szExtraInfoKey[MAX_WORD_LEN];
		for (map<string, string>::iterator it = pThreadInfo->m_mExInfo.begin(); it != pThreadInfo->m_mExInfo.end(); it++)
		{
			xpath_node_set tools = doc.select_nodes((*it).first.c_str());
			if (tools.size() == 0)
				cout << "Load [" << pThreadInfo->m_vFileList[i].c_str() << "] Extra Info NULL: " << (*it).second.c_str() << endl;

			fprintf(fpOut, "%s=", (*it).second.c_str());
			strcpy(szExtraInfoKey, tools.begin()->node().child_value());
			char* find = strchr(szExtraInfoKey, '.');
			if (find) {
				*find = *(find + 1);
				*(find + 1) = 0;  //在此处放置一个空字符
			}
			if (g_mExtraInfo.find(szExtraInfoKey) == g_mExtraInfo.end())
				cout << "Load [" << pThreadInfo->m_vFileList[i].c_str() << "] Extra Info NULL: Could not find" << szExtraInfoKey << endl;

			for (vector<string>::iterator itr = g_mExtraInfo[szExtraInfoKey].end() - 1; itr != g_mExtraInfo[szExtraInfoKey].begin(); itr--)
			{
				if (g_mExtraInfo[szExtraInfoKey].end() - itr > EXTRA_INFO_MAXNUM) break;
				fprintf(fpOut, "%s|", (*itr).c_str());
			}
			fprintf(fpOut, ";");
		}
		fprintf(fpOut, "\n");
		for (map<string, string>::iterator it = pThreadInfo->m_mField.begin(); it != pThreadInfo->m_mField.end(); it++)
		{
			pugi::xpath_node_set tools = doc.select_nodes((*it).first.c_str());
			if (tools.size() == 0)
				cout << "Load [" << pThreadInfo->m_vFileList[i].c_str() << "] Info NULL: " << (*it).second.c_str() << endl;

			// std::cout << (*it).first << endl;
			fprintf(fpOut, "<p>%s\n", (*it).second.c_str());
			for (pugi::xpath_node_set::const_iterator itr = tools.begin(); itr != tools.end(); ++itr)
			{
				simple_walker walker;
				(*itr).node().traverse(walker);
				fprintf(fpOut, "%s\n", walker.get().c_str());
			}
			fprintf(fpOut, "</p>\n");
		}
		fprintf(fpOut, "</#>\n");
	}
	fclose(fpOut);
}

bool GlobalInit(char* psConfig)
{
	return true;
}

class CXMLParser
{
public:
	CXMLParser();
	~CXMLParser();
	bool Initialize(char* psConfig, char* psOutPath) {
		m_Config.ConfigInit(psConfig);
		m_strOut = psOutPath;
		return true;
	}

	bool ReadList(char* psFileList, unsigned int nJobs) {
		FILE* fpInp = NULL;
		char szLine[MAX_PATH];
		szLine[0] = 0;
		if ((fpInp = fopen(psFileList, "rb")) == NULL) {
			printf("Error open %s\n", psFileList);
			return false;
		}
		while (fgets(szLine, MAX_PATH, fpInp) != NULL) {
			if (szLine[strlen(szLine) - 1] == '\n') {
				szLine[strlen(szLine) - 1] = 0;
			}
			if (szLine[strlen(szLine) - 1] == '\r') {
				szLine[strlen(szLine) - 1] = 0;
			}
			if (*szLine == ' ' || *szLine == 0 || *szLine == '#' || *szLine == '\\' || *szLine == '-') {
				continue;
			}
			m_vFileList.push_back(szLine);
		}
		fclose(fpInp);

		m_nJobs = nJobs;
		if (!SliceFileList(nJobs)) {
			return false;
		}
		return true;
	}

	bool SliceFileList(unsigned int nJobs)
	{
		if (m_vFileList.size() < nJobs) {
			m_nJobs = m_vFileList.size();
		}

		int nPerNum = m_vFileList.size() / m_nJobs;
		int nRemainNum = m_vFileList.size() % m_nJobs;
		if (nRemainNum > 0) nPerNum++;
		int nNo = 0;
		vector<string> m_vFileListTmp;
		for (unsigned int i = 0; i < m_nJobs; i++) {
			m_vFileListTmp.clear();
			for (int j = 0; j < nPerNum; j++) {
				m_vFileListTmp.push_back(m_vFileList[nNo]);
				nNo++;
			}
			m_vvFileList.push_back(m_vFileListTmp);
			if (m_vvFileList.size() >= nRemainNum && nPerNum == m_vFileList.size() / m_nJobs + 1) {
				nPerNum--;
			}
		}
		return true;
	}

	bool Run()
	{
		mkdir(m_strOut.c_str());

		thread_t** hThread = NULL;
		CThreadInfo* pThreadInfo;

		hThread = new thread_t* [m_nJobs];
		pThreadInfo = new CThreadInfo[m_nJobs];
		memset(hThread, 0, sizeof(thread_t*) * m_nJobs);

		printf("Creating XMLParser pool with [0 - %d] jobs...\n", m_nJobs);
		for (unsigned int i = 0; i < m_nJobs; i++) {
			pThreadInfo[i].m_strOutPath = m_strOut;
			pThreadInfo[i].m_nID = i;
			pThreadInfo[i].m_mField.insert(m_Config.m_mField.begin(), m_Config.m_mField.end());
			pThreadInfo[i].m_mInfo.insert(m_Config.m_mInfo.begin(), m_Config.m_mInfo.end());
			pThreadInfo[i].m_mExInfo.insert(m_Config.m_mExInfo.begin(), m_Config.m_mExInfo.end());
			for (unsigned int j = 0; j < m_vvFileList[i].size(); j++) pThreadInfo[i].m_vFileList.push_back(m_vvFileList[i][j]);
			hThread[i] = thread_create_ex((void*)&ThreadWorker, &pThreadInfo[i]);
		}

		for (unsigned int i = 0; i < m_nJobs; i++) {
			thread_join_ex(hThread[i], 0xFFFFFFFF);
		}

		delete[] hThread;
		delete[] pThreadInfo;
		return true;
	}
private:
	unsigned int m_nJobs;
	CConfig	m_Config;
	string m_strOut;
	vector<std::string> m_vFileList;	//all files 
	vector< vector<string> > m_vvFileList;	//file vector for thread worker
};

CXMLParser::CXMLParser()
{
}

CXMLParser::~CXMLParser()
{
}


int main(int argc, char** argv)
{
	if (argc != 6) {
		PrintUsage(argv[0]);
		return 0;
	}

	if (strcmp(argv[5], "NULL") != 0)
	{
		if (!ReadExtraInfo(argv[5])) {
			return 0;
		}
	}

	CXMLParser cxp;

	if (!cxp.Initialize(argv[1], argv[3])) {
		return false;
	}

	if (!cxp.ReadList(argv[2], atoi(argv[4]))) {
		return false;
	}

	try
	{
		cxp.Run();
	}
	catch (const std::exception&)
	{
		printf("catch");
	}

	return 0;
}
