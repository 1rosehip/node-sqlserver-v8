#include "stdafx.h"
#include "OdbcConnection.h"
#include "OdbcStatement.h"
#include "OdbcStatementCache.h"
#include "PrepareOperation.h"

namespace mssql
{
	PrepareOperation::PrepareOperation(
		shared_ptr<OdbcConnection> connection,
		const wstring& query,
		u_int id,
		u_int timeout,
		Handle<Object> callback) :
		QueryOperation(connection, query, id, timeout, callback)
	{
		persists = true;
	}

	bool PrepareOperation::TryInvokeOdbc()
	{
		statement = connection->statements->checkout(statementId);
		return statement->TryPrepare(query, timeout, params);
	}
}