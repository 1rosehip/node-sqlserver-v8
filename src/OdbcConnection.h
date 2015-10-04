//---------------------------------------------------------------------------------------------------------------------------------
// File: OdbcConnection.h
// Contents: Async calls to ODBC done in background thread
// 
// Copyright Microsoft Corporation and contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
//
// You may obtain a copy of the License at:
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//---------------------------------------------------------------------------------------------------------------------------------

#pragma once

#include "ResultSet.h"
#include "CriticalSection.h"
#include "OdbcOperation.h"
#include <map>

namespace mssql
{
    using namespace std;

    class OdbcConnection
    {
	   typedef bool (OdbcConnection::* dispatcher_p)(int);

	   static OdbcEnvironmentHandle environment;

	   OdbcConnectionHandle connection;
	   OdbcStatementHandle statement;
	   CriticalSection closeCriticalSection;

	   pair<SQLSMALLINT, dispatcher_p> getPair(SQLSMALLINT i, dispatcher_p p)
	   {
		  return pair<SQLSMALLINT, dispatcher_p>(i, p);
	   }

	   // any error that occurs when a Try* function returns false is stored here
	   // and may be retrieved via the Error function below.
	   shared_ptr<OdbcError> error;
	   typedef map<SQLSMALLINT, dispatcher_p> dispatcher_map;
	   dispatcher_map dispatchers;

	   void init();

	   bool d_String(int col);
	   bool d_Bit(int col);
	   bool d_Integer(int col);
	   bool d_Decimal(int col);
	   bool d_Binary(int col);
	   bool d_Timestamp(int col);
	   bool d_Time(int col);

	   enum ConnectionStates
	   {
		  Closed,
		  Opening,
		  TurnOffAutoCommit,
		  Open
	   } connectionState;

	   bool endOfResults;
	   bool BindParams(BoundDatumSet& params);
	   // set binary true if a binary Buffer should be returned instead of a JS string
	   bool TryReadString(bool binary, int column);

    public:
	   shared_ptr<ResultSet> resultset;

	   bool endRows() {
		  return  resultset != nullptr && resultset->EndOfRows();
	   }


	   OdbcConnection()
		  : error(nullptr),
		  connectionState(Closed),
		  endOfResults(true)
	   {
		  init();
	   }

	   static bool InitializeEnvironment();
	   bool readNext(int column);
	    bool StartReadingResults();

	   bool TryBeginTran();
	   bool TryClose();
	   bool TryOpen(const wstring& connectionString);
	   bool TryExecute(const wstring& query, BoundDatumSet& paramIt);
	   bool TryEndTran(SQLSMALLINT completionType);
	   bool TryReadRow();
	   bool TryReadColumn(int column);
	   bool Lob(int display_size, int column);
	   bool boundedString(int display_size, int column);
	   bool TryReadNextResult();

	   Handle<Value> GetMetaValue()
	   {
		  return resultset->MetaToValue();
	   }

	   Handle<Value> EndOfResults()
	   {
		  nodeTypeFactory fact;
		  return fact.newBoolean(endOfResults);
	   }

	   Handle<Value> EndOfRows()
	   {
		  nodeTypeFactory fact;
		  return fact.newBoolean(resultset->EndOfRows());
	   }

	   Handle<Value> GetColumnValue()
	   {
		  nodeTypeFactory fact;
		  auto result = fact.newObject();
		  auto column = resultset->GetColumn();
		  result->Set(fact.fromTwoByte(L"data"), column->ToValue());
		  result->Set(fact.fromTwoByte(L"more"), fact.newBoolean(column->More()));
		  return result;
	   }

	   shared_ptr<OdbcError> LastError(void)
	   {
		  return error;
	   }
    };
}