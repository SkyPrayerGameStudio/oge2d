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

#ifndef __OGE_DATABASE_H_INCLUDED__
#define __OGE_DATABASE_H_INCLUDED__

#ifndef SQLITE_HAS_CODEC
#define SQLITE_HAS_CODEC
#endif

#include <string>

#ifdef __OGE_WITH_DB__

#include "soci.h"
#include "soci-backend.h"

#if defined(__WIN32__) && !defined(__WINCE__)
#include "soci-odbc.h"
#endif

#ifdef __DB_WITH_SQLITE3__
#include "soci-sqlite3.h"
#endif

#ifdef __DB_WITH_MYSQL__
#include "soci-mysql.h"
#endif

#ifdef __DB_WITH_ORACLE__
#include "soci-oracle.h"
#endif

#ifdef __DB_WITH_POSTGRESQL__
#include "soci-postgresql.h"
#endif

#elif defined(__DB_WITH_SQLITE3__) && !defined(__OGE_WITH_DB__)

#include "sqlite3.h"

#endif



enum ogeDatabaseType
{
	DB_SQLITE3    = 1,
	DB_MYSQL      = 2,
	DB_ODBC       = 4,
	DB_ORACLE     = 8,
	DB_POSTGRESQL = 16
};

class CogeQuery;

class CogeDatabase
{
private:

#ifdef __OGE_WITH_DB__
    const soci::backend_factory* m_pSqliteBackend;
    const soci::backend_factory* m_pMysqlBackend;
    const soci::backend_factory* m_pOdbcBackend;
    const soci::backend_factory* m_pOracleBackend;
    const soci::backend_factory* m_pPostgresqlBackend;
#else
    void* m_pSqliteBackend;
    void* m_pMysqlBackend;
    void* m_pOdbcBackend;
    void* m_pOracleBackend;
    void* m_pPostgresqlBackend;
#endif

    size_t m_iValidTypes;

protected:

#ifdef __OGE_WITH_DB__
    const soci::backend_factory* GetValidBackend(size_t iDbType);
#else
    const void* GetValidBackend(size_t iDbType);
#endif

public:

    CogeDatabase();
    ~CogeDatabase();

    size_t GetValidTypes();

    bool IsValidType(size_t iDbType);

    int OpenDb(size_t iDbType, const std::string& sCnnStr);
    int RunSql(int iSessionId, const std::string& sSql);
    int OpenQuery(int iSessionId, const std::string& sQuerySql);
    void CloseQuery(int iQueryId);
    int FirstRecord(int iQueryId);
    int NextRecord(int iQueryId);
    bool GetBoolFieldValue(int iQueryId, const std::string& sFieldName);
    int GetIntFieldValue(int iQueryId, const std::string& sFieldName);
    double GetFloatFieldValue(int iQueryId, const std::string& sFieldName);
    std::string GetStrFieldValue(int iQueryId, const std::string& sFieldName);
    std::string GetTimeFieldValue(int iQueryId, const std::string& sFieldName);
    void CloseDb(int iSessionId);

    friend class CogeQuery;

};

class CogeQuery
{
private:

    void* m_pSession;

#ifdef __OGE_WITH_DB__
    soci::rowset<soci::row>* rslset;
    soci::rowset<soci::row>::const_iterator cursor;
    int recno;
    int colno;
#else
    void* rslset;
    void* cursor;
    int recno;
    int colno;
#endif

protected:

public:

    CogeQuery(int iSessionId, const std::string& sQuerySql);
    ~CogeQuery();

    int FirstRecord();
    int NextRecord();
    bool GetBoolFieldValue(const std::string& sFieldName);
    int GetIntFieldValue(const std::string& sFieldName);
    double GetFloatFieldValue(const std::string& sFieldName);
    std::string GetStrFieldValue(const std::string& sFieldName);
    std::string GetTimeFieldValue(const std::string& sFieldName);

    friend class CogeDatabase;

};




#endif // __OGE_DATABASE_H_INCLUDED__
