#include "stdafx.h"
#include <BoundDatum.h>
#include <BoundDatumSet.h>
#include <ResultSet.h>

namespace mssql
{
	BoundDatumSet::BoundDatumSet() :
		err(nullptr),
		first_error(0),
		_output_param_count(-1)
	{
		_bindings = make_shared<param_bindings>();
	}

	bool BoundDatumSet::reserve(const shared_ptr<ResultSet>& set, const size_t row_count) const
	{
		for (uint32_t i = 0; i < set->get_column_count(); ++i) {
			const auto binding = make_shared<BoundDatum>();
			const auto& def = set->get_meta_data(i);
			binding->reserve_column_type(def.dataType, def.columnSize, row_count);
			_bindings->push_back(binding);
		}
		return true;
	}

	Local<Value> get(Local<Object> o, const char* v)
	{
		const nodeTypeFactory fact;
		const auto context = fact.isolate->GetCurrentContext();
		const auto vp = fact.new_string(v);
		const auto maybe = o->Get(context, vp);
		Local<Value> val;
		if (maybe.ToLocal(&val)) {
			return val;
		}
		return val;
	}

	int get_tvp_col_count(Local<Value>& v)
	{
		const auto tvp_columns = get(v.As<Object>(), "table_value_param");
		const auto cols = tvp_columns.As<Array>();
		const auto count = cols->Length();
		return count;
	}

	bool BoundDatumSet::tvp(Local<Value>& v) const
	{
		const auto tvp_columns = get(v.As<Object>(), "table_value_param");
		if (tvp_columns->IsNull()) return false;
		if (!tvp_columns->IsArray()) return false;

		const auto cols = tvp_columns.As<Array>();
		const auto count = cols->Length();
		const nodeTypeFactory fact;
		const auto context = fact.isolate->GetCurrentContext();

		for (uint32_t i = 0; i < count; ++i) {
			const auto binding = make_shared<BoundDatum>();
			auto maybe = cols->Get(context, i);
			Local<Value> local;
			auto res = maybe.ToLocal(&local);
			if (!res) break;
			res = binding->bind(local);
			if (!res) break;
			_bindings->push_back(binding);
		}
		return true;
	}

	bool BoundDatumSet::bind(Local<Array>& node_params)
	{
		const nodeTypeFactory fact;
		const auto context = fact.isolate->GetCurrentContext();
		const auto count = node_params->Length();
		auto res = true;
		_output_param_count = 0;
		if (count > 0) {
			for (uint32_t i = 0; i < count; ++i) {
				const auto binding = make_shared<BoundDatum>();
				auto maybe = node_params->Get(context, i);
				Local<Value> local;
				if (maybe.ToLocal(&local)) {
					res = binding->bind(local);

					switch (binding->param_type)
					{
					case SQL_PARAM_OUTPUT:
					case SQL_PARAM_INPUT_OUTPUT:
						_output_param_count++;
						break;

					default:
						break;
					}

					if (!res) {
						err = binding->getErr();
						first_error = i;
						break;
					}

					_bindings->push_back(binding);

					if (binding->is_tvp)
					{
						const auto col_count = get_tvp_col_count(local);
						binding->tvp_no_cols = col_count;
						res = tvp(local);
					}
				} else
				{
					res = false;
				}
			}
		}

		return res;
	}

	Local<Array> BoundDatumSet::unbind()
	{
		const nodeTypeFactory fact;
		const auto context = fact.isolate->GetCurrentContext();
		const auto arr = fact.new_array(_output_param_count);
		auto i = 0;

		std::for_each(_bindings->begin(), _bindings->end(), [&](shared_ptr<BoundDatum> & param) mutable
			{
				switch (param->param_type)
				{
				case SQL_PARAM_OUTPUT:
				case SQL_PARAM_INPUT_OUTPUT:
				{
					const auto v = param->unbind();
					arr->Set(context, i++, v);
				}
				break;

				default:
					break;
				}
			});
		return arr;
	}
}