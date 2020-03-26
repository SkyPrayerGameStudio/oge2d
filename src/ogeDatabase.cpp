/*
-----------------------------------------------------------------------------
This source file is part of Open Game Engine 2D.
It is licensed under the terms of the MIT license.
For the latest info, see http://oge2d.sourceforge.net

Copyright (c) 2010-2012 Lin Jia Jun (Joe Lam)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "ogeDatabase.h"
#include "ogeCommon.h"

#include <cstdio>
#include <ctime>

#ifdef __OGE_WITH_DB__

/*------------------ CogeDatabase ------------------*/

CogeDatabase::CogeDatabase()
{
    m_iValidTypes = 0;

#if defined(__WIN32__) && !defined(__WINCE__)
    m_pOdbcBackend = soci::factory_odbc();
    m_iValidTypes = m_iValidTypes | DB_ODBC;
#else
    m_pOdbcBackend = NULL;
#endif

#ifdef __DB_WITH_SQLITE3__
    m_pSqliteBackend = soci::factory_sqlite3();
    m_iValidTypes = m_iValidTypes | DB_SQLITE3;
#else
    m_pSqliteBackend = NULL;
#endif

#ifdef __DB_WITH_MYSQL__
    m_pMysqlBackend = soci::factory_mysql();
    m_iValidTypes = m_iValidTypes | DB_MYSQL;
#else
    m_pMysqlBackend = NULL;
#endif

#ifdef __DB_WITH_ORACLE__
    m_pOracleBackend = soci::factory_oracle();
    m_iValidTypes = m_iValidTypes | DB_ORACLE;
#else
    m_pOracleBackend = NULL;
#endif

#ifdef __DB_WITH_POSTGRESQL__
    m_pPostgresqlBackend = soci::factory_postgresql();
    m_iValidTypes = m_iValidTypes | DB_POSTGRESQL;
#else
    m_pPostgresqlBackend = NULL;
#endif

}


CogeDatabase::~CogeDatabase()
{
    m_iValidTypes = 0;
    //if(m_pOdbcBackend) delete m_pOdbcBackend;
    //if(m_pSqliteBackend) delete m_pSqliteBackend;
    //if(m_pMysqlBackend) delete m_pMysqlBackend;
    //if(m_pOracleBackend) delete m_pOracleBackend;
    //if(m_pPostgresqlBackend) delete m_pPostgresqlBackend;
}

size_t CogeDatabase::GetValidTypes()
{
    return m_iValidTypes;
}

bool CogeDatabase::IsValidType(size_t iDbType)
{
    return (m_iValidTypes & iDbType) != 0;
}

const soci::backend_factory* CogeDatabase::GetValidBackend(size_t iDbType)
{
    //if(!IsValidType(iDbType)) return NULL;

    switch(iDbType)
    {
    case DB_SQLITE3: return m_pSqliteBackend;
    case DB_MYSQL: return m_pMysqlBackend;
    case DB_ODBC: return m_pOdbcBackend;
    case DB_ORACLE: return m_pOracleBackend;
    case DB_POSTGRESQL: return m_pPostgresqlBackend;
    default: return NULL;
    }

    //return NULL;
}

int CogeDatabase::OpenDb(size_t iDbType, const std::string& sCnnStr)
{
    const soci::backend_factory* pBackend = GetValidBackend(iDbType);
    if(pBackend == NULL) return 0;

    int rsl = 0;
    try
    {
        soci::session* pSession = new soci::session(*pBackend, sCnnStr);
        rsl = (int) pSession;
    }
    catch (soci::soci_error const &e)
    {
        OGE_Log("Error in OpenDb(): %s \n", e.what());
        rsl = 0;
    }

    return rsl;
}

int CogeDatabase::RunSql(int iSessionId, const std::string& sSql)
{
    if(iSessionId == 0) return -1;
    soci::session* pSession = (soci::session*) iSessionId;

    int rsl = -1;
    try
    {
        (*pSession) << sSql;
        rsl = 0;
    }
    catch (soci::soci_error const &e)
    {
        OGE_Log("Error in RunSql(): %s \n", e.what());
        OGE_Log("SQL: %s \n", sSql.c_str());
        rsl = -1;
    }

    return rsl;
}

int CogeDatabase::OpenQuery(int iSessionId, const std::string& sQuerySql)
{
    if(iSessionId == 0) return 0;

    int rsl = 0;
    try
    {
        CogeQuery* pQuery = new CogeQuery(iSessionId, sQuerySql);
        rsl = (int) pQuery;
    }
    catch (soci::soci_error const &e)
    {
        OGE_Log("Error in OpenQuery(): %s \n", e.what());
        OGE_Log("QuerySQL: %s \n", sQuerySql.c_str());
        rsl = 0;
    }

    return rsl;
}

void CogeDatabase::CloseQuery(int iQueryId)
{
    if(iQueryId != 0)
    {
        CogeQuery* pQuery = (CogeQuery*)iQueryId;
        if(pQuery) delete pQuery;
    }
}

int CogeDatabase::FirstRecord(int iQueryId)
{
    if(iQueryId == 0) return 0;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->FirstRecord();
}

int CogeDatabase::NextRecord(int iQueryId)
{
    if(iQueryId == 0) return 0;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->NextRecord();
}

bool CogeDatabase::GetBoolFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return false;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetBoolFieldValue(sFieldName);
}

int CogeDatabase::GetIntFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return 0;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetIntFieldValue(sFieldName);
}

double CogeDatabase::GetFloatFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return 0;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetFloatFieldValue(sFieldName);
}

std::string CogeDatabase::GetStrFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return "";
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetStrFieldValue(sFieldName);
}

std::string CogeDatabase::GetTimeFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return "";
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetTimeFieldValue(sFieldName);
}

void CogeDatabase::CloseDb(int iSessionId)
{
    if(iSessionId == 0) return;
    soci::session* pSession = (soci::session*) iSessionId;
    if(pSession) delete pSession;
}


/*------------------ CogeQuery ------------------*/

CogeQuery::CogeQuery(int iSessionId, const std::string& sQuerySql)
{
    recno = -1;
    colno = -1;
    rslset = NULL;
    m_pSession = NULL;
    if(iSessionId != 0)
    {
        soci::session* pSession = (soci::session*) iSessionId;
        m_pSession = (void*) pSession;
        rslset = new soci::rowset<soci::row>((*pSession).prepare << sQuerySql);
        recno = 0;
    }

}
CogeQuery::~CogeQuery()
{
    recno = -1;
    if(rslset) delete rslset;
    rslset = NULL;
    m_pSession = NULL;
}
int CogeQuery::FirstRecord()
{
    if(recno < 0 || !rslset) return 0;
    cursor = rslset->begin();
    if(cursor == rslset->end()) return 0;
    recno = 1;
    return recno;
}
int CogeQuery::NextRecord()
{
    if(recno <= 0 || !rslset) return 0;
    if(cursor == rslset->end()) return 0;
    cursor++;
    if(cursor == rslset->end()) return 0;
    recno++;
    return recno;
}
bool CogeQuery::GetBoolFieldValue(const std::string& sFieldName)
{
    if(recno <= 0 || !rslset) return false;
    if(cursor == rslset->end()) return false;

    soci::row const& r = *cursor;
    return r.get<bool>(sFieldName);
}
int CogeQuery::GetIntFieldValue(const std::string& sFieldName)
{
    if(recno <= 0 || !rslset) return 0;
    if(cursor == rslset->end()) return 0;

    soci::row const& r = *cursor;
    return r.get<int>(sFieldName);
}
double CogeQuery::GetFloatFieldValue(const std::string& sFieldName)
{
    if(recno <= 0 || !rslset) return 0;
    if(cursor == rslset->end()) return 0;

    soci::row const& r = *cursor;
    return r.get<double>(sFieldName);
}
std::string CogeQuery::GetStrFieldValue(const std::string& sFieldName)
{
    if(recno <= 0 || !rslset) return "";
    if(cursor == rslset->end()) return "";

    soci::row const& r = *cursor;
    return r.get<std::string>(sFieldName);
}
std::string CogeQuery::GetTimeFieldValue(const std::string& sFieldName)
{
    if(recno <= 0 || !rslset) return "";
    if(cursor == rslset->end()) return "";

    soci::row const& r = *cursor;
    std::tm when = r.get<std::tm>(sFieldName);

    std::string rsl;
    char buffer [20];

    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", &when);

    rsl = buffer;

    return rsl;
}


#elif defined(__DB_WITH_SQLITE3__) && !defined(__OGE_WITH_DB__)



/*------------------ CogeDatabase ------------------*/

CogeDatabase::CogeDatabase()
{
    m_iValidTypes = 0;
    m_pOdbcBackend = NULL;
    m_pSqliteBackend = NULL;
    m_pMysqlBackend = NULL;
    m_pOracleBackend = NULL;
    m_pPostgresqlBackend = NULL;

    m_iValidTypes = m_iValidTypes | DB_SQLITE3;

    //printf("Database functions are not available. \n");
}


CogeDatabase::~CogeDatabase()
{
    m_iValidTypes = 0;
    //if(m_pOdbcBackend) delete m_pOdbcBackend;
    //if(m_pSqliteBackend) delete m_pSqliteBackend;
    //if(m_pMysqlBackend) delete m_pMysqlBackend;
    //if(m_pOracleBackend) delete m_pOracleBackend;
    //if(m_pPostgresqlBackend) delete m_pPostgresqlBackend;
}

size_t CogeDatabase::GetValidTypes()
{
    return m_iValidTypes;
}

bool CogeDatabase::IsValidType(size_t iDbType)
{
    return (m_iValidTypes & iDbType) != 0;
}

const void* CogeDatabase::GetValidBackend(size_t iDbType)
{
    //if(!IsValidType(iDbType)) return NULL;

    switch(iDbType)
    {
    case DB_SQLITE3: return m_pSqliteBackend;
    case DB_MYSQL: return m_pMysqlBackend;
    case DB_ODBC: return m_pOdbcBackend;
    case DB_ORACLE: return m_pOracleBackend;
    case DB_POSTGRESQL: return m_pPostgresqlBackend;
    default: return NULL;
    }

    //return NULL;
}

int CogeDatabase::OpenDb(size_t iDbType, const std::string& sCnnStr)
{
    if(iDbType == DB_SQLITE3)
    {
        sqlite3* pDB = NULL;

        int rsl = sqlite3_open(sCnnStr.c_str(), &pDB);

        if (rsl != SQLITE_OK)
        {
            const char* szError = sqlite3_errmsg(pDB);
            OGE_Log("Fail to open database: %s. \n", szError);
            return 0;
        }
        else
        {
            m_pSqliteBackend = (void*) pDB;
            return (int) m_pSqliteBackend;
        }

    }
    else
    {
        OGE_Log("Database functions are not available. \n");
        return 0;
    }

}

int CogeDatabase::RunSql(int iSessionId, const std::string& sSql)
{
    if(iSessionId == 0 || sSql.length() == 0) return -1;

    sqlite3* pDB = (sqlite3*) iSessionId;

    char *zErrMsg = 0;
    int const res = sqlite3_exec(pDB, sSql.c_str(), 0, 0, &zErrMsg);

    if (res == SQLITE_OK)
    {
        return sqlite3_changes(pDB);
    }
    else
    {
        OGE_Log("Fail to execute sql: %s \n", sSql.c_str());
        OGE_Log("Error: %s \n", zErrMsg);
        return -1;
    }
}

int CogeDatabase::OpenQuery(int iSessionId, const std::string& sQuerySql)
{
    if(iSessionId == 0) return 0;

    int rsl = 0;

    CogeQuery* pQuery = new CogeQuery(iSessionId, sQuerySql);

    if(pQuery->recno < 0)
    {
        delete pQuery;
        pQuery = NULL;
    }

    rsl = (int) pQuery;

    return rsl;
}

void CogeDatabase::CloseQuery(int iQueryId)
{
    if(iQueryId != 0)
    {
        CogeQuery* pQuery = (CogeQuery*)iQueryId;
        if(pQuery) delete pQuery;
    }
}

int CogeDatabase::FirstRecord(int iQueryId)
{
    if(iQueryId == 0) return 0;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->FirstRecord();
}

int CogeDatabase::NextRecord(int iQueryId)
{
    if(iQueryId == 0) return 0;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->NextRecord();
}

bool CogeDatabase::GetBoolFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return false;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetBoolFieldValue(sFieldName);
}

int CogeDatabase::GetIntFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return 0;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetIntFieldValue(sFieldName);
}

double CogeDatabase::GetFloatFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return 0;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetFloatFieldValue(sFieldName);
}

std::string CogeDatabase::GetStrFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return "";
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetStrFieldValue(sFieldName);
}

std::string CogeDatabase::GetTimeFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return "";
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetTimeFieldValue(sFieldName);
}

void CogeDatabase::CloseDb(int iSessionId)
{
    if(iSessionId == 0) return;

    sqlite3* pDB = (sqlite3*) iSessionId;

    if (sqlite3_close(pDB) == SQLITE_OK)
    {
        pDB = 0;
        m_pSqliteBackend = NULL;
    }
    else
    {
        OGE_Log("Fail to close database. \n");
    }
}


/*------------------ CogeQuery ------------------*/

CogeQuery::CogeQuery(int iSessionId, const std::string& sQuerySql)
{
    recno = -1;
    colno = -1;
    rslset = NULL;
    cursor = NULL;
    m_pSession = NULL;

    if(iSessionId != 0)
    {
        sqlite3* pDB = (sqlite3*) iSessionId;
        m_pSession = (void*) pDB;

        const char* szMsg=0;
        sqlite3_stmt* pStmt;

        int nRet = sqlite3_prepare_v2(pDB, sQuerySql.c_str(), -1, &pStmt, &szMsg);

        if (nRet == SQLITE_OK)
        {
            rslset = (void*) pStmt;
            recno = 0;
        }
        else
        {
            const char* szError = sqlite3_errmsg(pDB);
            OGE_Log("Fail to open query: %s \n", sQuerySql.c_str());
            OGE_Log("Error: %s \n", szError);
        }
    }


}
CogeQuery::~CogeQuery()
{
    if(rslset)
    {
        sqlite3_stmt* pStmt = (sqlite3_stmt*) rslset;
        sqlite3_finalize(pStmt);
    }

    recno = -1;
    rslset = NULL;
    m_pSession = NULL;
}
int CogeQuery::FirstRecord()
{
    if(recno < 0 || !rslset) return 0;

    sqlite3_stmt* pStmt = (sqlite3_stmt*) rslset;

    int nRet = -1;

    if(recno > 0)
    {
        nRet = sqlite3_reset(pStmt);
        if(nRet == SQLITE_OK)
        {
            recno = 0;
            cursor = NULL;
        }
        else
        {
            if(m_pSession)
            {
                sqlite3* pDB = (sqlite3*) m_pSession;
                const char* szError = sqlite3_errmsg(pDB);
                OGE_Log("Fail to reset cursor: %s \n", szError);
            }
            return 0;
        }
    }

    nRet = sqlite3_step(pStmt);

	if (nRet == SQLITE_DONE)
	{
	    cursor = (void*)(&recno);
		return 0;
	}
	else if (nRet == SQLITE_ROW)
	{
	    if(colno < 0) colno = sqlite3_column_count(pStmt);

		recno = 1;
		cursor = NULL;
		return recno;
	}
	else
	{
	    if(m_pSession)
        {
            sqlite3* pDB = (sqlite3*) m_pSession;
            const char* szError = sqlite3_errmsg(pDB);
            OGE_Log("Fail to move cursor: %s \n", szError);
        }
        return 0;
	}

}
int CogeQuery::NextRecord()
{
    if(recno < 0 || !rslset) return 0;

    if(cursor != NULL) return 0;

    sqlite3_stmt* pStmt = (sqlite3_stmt*) rslset;

    int nRet = -1;

    nRet = sqlite3_step(pStmt);

	if (nRet == SQLITE_DONE)
	{
	    cursor = (void*)(&recno);
		return 0;
	}
	else if (nRet == SQLITE_ROW)
	{
	    if(colno < 0) colno = sqlite3_column_count(pStmt);

		recno++;
		cursor = NULL;
		return recno;
	}
	else
	{
	    if(m_pSession)
        {
            sqlite3* pDB = (sqlite3*) m_pSession;
            const char* szError = sqlite3_errmsg(pDB);
            OGE_Log("Fail to move cursor: %s \n", szError);
        }
        return 0;
	}
}
bool CogeQuery::GetBoolFieldValue(const std::string& sFieldName)
{
    if(recno <= 0 || !rslset) return false;
    if(cursor != NULL || colno <= 0) return false;

    sqlite3_stmt* pStmt = (sqlite3_stmt*) rslset;

    int iFieldIndex = -1;

    if (sFieldName.length() > 0)
	{
		for (int nField = 0; nField < colno; nField++)
		{
			const char* szTemp = sqlite3_column_name(pStmt, nField);

			if (sFieldName.compare(szTemp) == 0)
			{
				iFieldIndex = nField;
				break;
			}
		}
	}

	if(iFieldIndex >= 0)
    {
        if(sqlite3_column_type(pStmt, iFieldIndex) == SQLITE_NULL) return false;

        return sqlite3_column_int(pStmt, iFieldIndex) != 0;

    }

    return false;

}
int CogeQuery::GetIntFieldValue(const std::string& sFieldName)
{
    if(recno <= 0 || !rslset) return 0;
    if(cursor != NULL || colno <= 0) return 0;

    sqlite3_stmt* pStmt = (sqlite3_stmt*) rslset;

    int iFieldIndex = -1;

    if (sFieldName.length() > 0)
	{
		for (int nField = 0; nField < colno; nField++)
		{
			const char* szTemp = sqlite3_column_name(pStmt, nField);

			if (sFieldName.compare(szTemp) == 0)
			{
				iFieldIndex = nField;
				break;
			}
		}
	}

	if(iFieldIndex >= 0)
    {
        if(sqlite3_column_type(pStmt, iFieldIndex) == SQLITE_NULL) return 0;

        return sqlite3_column_int(pStmt, iFieldIndex);

    }

    return 0;
}
double CogeQuery::GetFloatFieldValue(const std::string& sFieldName)
{
    if(recno <= 0 || !rslset) return 0;
    if(cursor != NULL || colno <= 0) return 0;

    sqlite3_stmt* pStmt = (sqlite3_stmt*) rslset;

    int iFieldIndex = -1;

    if (sFieldName.length() > 0)
	{
		for (int nField = 0; nField < colno; nField++)
		{
			const char* szTemp = sqlite3_column_name(pStmt, nField);

			if (sFieldName.compare(szTemp) == 0)
			{
				iFieldIndex = nField;
				break;
			}
		}
	}

	if(iFieldIndex >= 0)
    {
        if(sqlite3_column_type(pStmt, iFieldIndex) == SQLITE_NULL) return 0;

        return sqlite3_column_double(pStmt, iFieldIndex);

    }

    return 0;
}
std::string CogeQuery::GetStrFieldValue(const std::string& sFieldName)
{
    std::string rsl = "";
    if(recno <= 0 || !rslset) return rsl;
    if(cursor != NULL || colno <= 0) return rsl;

    sqlite3_stmt* pStmt = (sqlite3_stmt*) rslset;

    int iFieldIndex = -1;

    if (sFieldName.length() > 0)
	{
		for (int nField = 0; nField < colno; nField++)
		{
			const char* szTemp = sqlite3_column_name(pStmt, nField);

			if (sFieldName.compare(szTemp) == 0)
			{
				iFieldIndex = nField;
				break;
			}
		}
	}

	if(iFieldIndex >= 0)
    {
        if(sqlite3_column_type(pStmt, iFieldIndex) == SQLITE_NULL) return rsl;
        const char* szRsl = (const char*) sqlite3_column_text(pStmt, iFieldIndex);
        if(szRsl) rsl = szRsl;

    }

    return rsl;
}
std::string CogeQuery::GetTimeFieldValue(const std::string& sFieldName)
{
    return GetStrFieldValue(sFieldName);
}




#else





/*------------------ CogeDatabase ------------------*/

CogeDatabase::CogeDatabase()
{
    m_iValidTypes = 0;
    m_pOdbcBackend = NULL;
    m_pSqliteBackend = NULL;
    m_pMysqlBackend = NULL;
    m_pOracleBackend = NULL;
    m_pPostgresqlBackend = NULL;

    //printf("Database functions are not available. \n");
}


CogeDatabase::~CogeDatabase()
{
    m_iValidTypes = 0;
    //if(m_pOdbcBackend) delete m_pOdbcBackend;
    //if(m_pSqliteBackend) delete m_pSqliteBackend;
    //if(m_pMysqlBackend) delete m_pMysqlBackend;
    //if(m_pOracleBackend) delete m_pOracleBackend;
    //if(m_pPostgresqlBackend) delete m_pPostgresqlBackend;
}

size_t CogeDatabase::GetValidTypes()
{
    return m_iValidTypes;
}

bool CogeDatabase::IsValidType(size_t iDbType)
{
    return (m_iValidTypes & iDbType) != 0;
}

const void* CogeDatabase::GetValidBackend(size_t iDbType)
{
    //if(!IsValidType(iDbType)) return NULL;

    switch(iDbType)
    {
    case DB_SQLITE3: return m_pSqliteBackend;
    case DB_MYSQL: return m_pMysqlBackend;
    case DB_ODBC: return m_pOdbcBackend;
    case DB_ORACLE: return m_pOracleBackend;
    case DB_POSTGRESQL: return m_pPostgresqlBackend;
    default: return NULL;
    }

    //return NULL;
}

int CogeDatabase::OpenDb(size_t iDbType, const std::string& sCnnStr)
{
    OGE_Log("Database functions are not available. \n");
    return 0;
}

int CogeDatabase::RunSql(int iSessionId, const std::string& sSql)
{
    return -1;
}

int CogeDatabase::OpenQuery(int iSessionId, const std::string& sQuerySql)
{
    return 0;
}

void CogeDatabase::CloseQuery(int iQueryId)
{
    if(iQueryId != 0)
    {
        CogeQuery* pQuery = (CogeQuery*)iQueryId;
        if(pQuery) delete pQuery;
    }
}

int CogeDatabase::FirstRecord(int iQueryId)
{
    if(iQueryId == 0) return 0;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->FirstRecord();
}

int CogeDatabase::NextRecord(int iQueryId)
{
    if(iQueryId == 0) return 0;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->NextRecord();
}

bool CogeDatabase::GetBoolFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return false;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetBoolFieldValue(sFieldName);
}

int CogeDatabase::GetIntFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return 0;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetIntFieldValue(sFieldName);
}

double CogeDatabase::GetFloatFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return 0;
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetFloatFieldValue(sFieldName);
}

std::string CogeDatabase::GetStrFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return "";
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetStrFieldValue(sFieldName);
}

std::string CogeDatabase::GetTimeFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(iQueryId == 0) return "";
    CogeQuery* pQuery = (CogeQuery*)iQueryId;
    return pQuery->GetTimeFieldValue(sFieldName);
}

void CogeDatabase::CloseDb(int iSessionId)
{
    return;
}


/*------------------ CogeQuery ------------------*/

CogeQuery::CogeQuery(int iSessionId, const std::string& sQuerySql)
{
    recno = -1;
    colno = -1;
    rslset = NULL;
    cursor = NULL;
    m_pSession = NULL;
}
CogeQuery::~CogeQuery()
{
    recno = -1;
    rslset = NULL;
    m_pSession = NULL;
}
int CogeQuery::FirstRecord()
{
    return recno;
}
int CogeQuery::NextRecord()
{
    return recno;
}
bool CogeQuery::GetBoolFieldValue(const std::string& sFieldName)
{
    return false;
}
int CogeQuery::GetIntFieldValue(const std::string& sFieldName)
{
    return 0;
}
double CogeQuery::GetFloatFieldValue(const std::string& sFieldName)
{
    return 0;
}
std::string CogeQuery::GetStrFieldValue(const std::string& sFieldName)
{
    return "";
}
std::string CogeQuery::GetTimeFieldValue(const std::string& sFieldName)
{
    return "";
}





#endif

