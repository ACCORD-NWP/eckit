/*
 * (C) Copyright 1996-2012 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "eckit/utils/Translator.h"
#include "eckit/types/Types.h"

//#include "odb_api/DispatchingWriter.h"
//#include "odb_api/SQLODAOutput.h"
//#include "odb_api/TemplateParameters.h"
//#include "odb_api/Writer.h"

#include "eckit/sql/expression/BitColumnExpression.h"
#include "eckit/sql/expression/ShiftedColumnExpression.h"
#include "eckit/sql/expression/SQLExpressions.h"
#include "eckit/sql/expression/function/FunctionExpression.h"
//#include "eckit/sql/SQLDistinctOutput.h"
//#include "eckit/sql/SQLOrderOutput.h"
#include "eckit/sql/SQLOutputConfig.h"
#include "eckit/sql/SQLSelectFactory.h"
#include "eckit/sql/SQLSelect.h"
#include "eckit/sql/SQLSession.h"


namespace eckit {
namespace sql {

//----------------------------------------------------------------------------------------------------------------------

SQLSelectFactory::SQLSelectFactory(SQLSession& session) :
    session_(session),
    implicitFromTableSource_(0),
    implicitFromTableSourceStream_(0),
    database_(0),
    maxColumnShift_(0),
    minColumnShift_(0) {}

/*void SQLSelectFactory::reset()
{
    // TODO> we may need to delete things here...
    implicitFromTableSource_ = 0;
    implicitFromTableSourceStream_ = 0;
    database_ = 0;
    config_ = SQLOutputConfig::defaultConfig();
    maxColumnShift_ = 0;
    minColumnShift_ = 0;
}
*/

std::string SQLSelectFactory::index(const std::string& columnName, const SQLExpression* index)
{
	if (index == 0)
		return columnName;

	bool missing = false;
    std::string idx = Translator<int,std::string>()(int(index->eval(missing)));
	ASSERT(! missing);
	return columnName + "_" + idx;
}

std::shared_ptr<SQLExpression> SQLSelectFactory::createColumn(
    const std::string& columnName,
    const std::string& bitfieldName,
    std::shared_ptr<SQLExpression>& vectorIndex,
    const std::string& tableReference,
    std::shared_ptr<SQLExpression>& pshift)
{
	if (! pshift->isConstant()) throw eckit::UserError("Value of shift operator must be constant");
	bool missing = false;
	
	// Internally shift is an index in the cyclic buffer of old values, so the shift value is negative.
	int shift = - pshift->eval(missing);

	if (shift > maxColumnShift_) maxColumnShift_ = shift;
	if (shift < minColumnShift_) minColumnShift_ = shift;

    std::string expandedColumnName( index(columnName, vectorIndex.get()) );
    // TODO: handle .<database>
	return bitfieldName.size()
        ? (shift == 0 ? std::make_shared<BitColumnExpression>(expandedColumnName, bitfieldName, tableReference)
                      : std::make_shared<ShiftedColumnExpression<BitColumnExpression>>(expandedColumnName, bitfieldName, tableReference, shift, -shift))
        : (shift == 0 ? std::make_shared<ColumnExpression>(expandedColumnName + tableReference, tableReference)
                      : std::make_shared<ShiftedColumnExpression<ColumnExpression>>(expandedColumnName + tableReference, tableReference, shift, -shift));
}

std::shared_ptr<SQLExpression> SQLSelectFactory::reshift(std::shared_ptr<SQLExpression>& e)
{
    if (!e) return e;

    ShiftedColumnExpression<BitColumnExpression>* c1 = dynamic_cast<ShiftedColumnExpression<BitColumnExpression>*>(e.get());
    if (c1) {
        int newShift = c1->shift() - minColumnShift_;
        ASSERT(newShift >= 0);
        if (newShift > 0) {
            return std::make_shared<ShiftedColumnExpression<BitColumnExpression>>(*c1, newShift, c1->nominalShift());
        } else {
            auto r = std::make_shared<BitColumnExpression>(*c1);
            r->nominalShift(c1->nominalShift());
            return r;
        }
    } 

    ShiftedColumnExpression<ColumnExpression>* c2 = dynamic_cast<ShiftedColumnExpression<ColumnExpression>*>(e.get());
    if (c2) {
        int newShift = c2->shift() - minColumnShift_ ;
        ASSERT(newShift >= 0);
        if (newShift > 0) {
            return std::make_shared<ShiftedColumnExpression<ColumnExpression>>(*c2, newShift, c2->nominalShift());
        } else {
            auto r = std::make_shared<ColumnExpression>(*c2);
            r->nominalShift(c2->nominalShift());
            return r;
        }
    } 

    BitColumnExpression* c3 = dynamic_cast<BitColumnExpression*>(e.get());
    if(c3) {
        return std::make_shared<ShiftedColumnExpression<BitColumnExpression>>(*c3, -minColumnShift_, 0);
    }

    ColumnExpression* c4 = dynamic_cast<ColumnExpression*>(e.get());
    if(c4) {
        return std::make_shared<ShiftedColumnExpression<ColumnExpression>>(*c4, -minColumnShift_, 0);
    }
    
    expression::function::FunctionExpression* f = dynamic_cast<expression::function::FunctionExpression*>(e.get());
    if (f) {
        reshift(f->args());
        return e;
    }

    Log::info() << "SQLSelectFactory::reshift: SKIP " << *e << std::endl;
    return e;
}

void SQLSelectFactory::reshift(Expressions& select)
{
    std::ostream& L(Log::debug());
    L << "reshift: maxColumnShift_ = " << maxColumnShift_ << std::endl;
    L << "reshift: minColumnShift_ = " << minColumnShift_ << std::endl;
	for (size_t i = 0; i < select.size(); ++i)
        L << "reshift: <- select[" << i << "]=" << *select[i] << std::endl;

	for (size_t i = 0; i < select.size(); ++i)
        select[i] = reshift(select[i]);

    L << std::endl;
	for (size_t i = 0; i < select.size(); ++i)
        L << "reshift: -> select[" << i << "]=" << *select[i] << std::endl;
}

//std::vector<SQLTable*> SQLSelectFactory::resolveImplicitFrom(SQLSession& session, std::vector<Table>& from)
//{
//    std::ostream& L (Log::debug());
//
//    L << "No <from> clause" << std::endl;
//
//    // TODO: SQLTable => std::string.
//
//    SQLTable* table = implicitFromTableSource_ ? session.openDataHandle(*implicitFromTableSource_)
//        : implicitFromTableSourceStream_ ? session.openDataStream(*implicitFromTableSourceStream_, csvDelimiter_)
//        //: database_ ? database_->table("defaultTable")
//        : database_ ? database_->defaultTable()
//        : session.currentDatabase().dualTable();
//
//    L << "Implicit FROM: " << *table << std::endl;
//
//    std::vector<SQLTable*> fromTables;
//    fromTables.push_back(table);
//    return fromTables;
//}

/*
SchemaAnalyzer& SQLSelectFactory::analyzer()
{ return SQLSession::current().currentDatabase().schemaAnalyzer(); }

MetaData SQLSelectFactory::columns(const std::string& tableName)
{
    const TableDef& tabledef ( enalyzer().findTable(tableName) );
    const ColumnDefs& columnDefs ( tabledef.columns() );

    //TODO: Convert ColumnDefs (from tabledef) into MetaData and push it into the SQLODAOutput
    ASSERT( false ); /// @todo this code must be fixed and return
}
*/


SQLSelect* SQLSelectFactory::create (
    bool distinct,
    bool all,
    const Expressions& select_list,
    const std::string& into,
    const std::vector<std::reference_wrapper<SQLTable>>& from)
//    SQLExpression *where,
//    Expressions group_by,
//    std::pair<Expressions,std::vector<bool> > order_by)
{
    std::ostream& L(Log::debug());

    ////// if (where) L << "SQLSelectFactory::create: where = " << *where << std::endl;

	SQLSelect* r (0);
	//SQLSession& session (SQLSession::current());

    std::vector<std::reference_wrapper<SQLTable>> fromTables;

    // TODO: Implicit tables
    ////if (! from.size())
 ////   {
 ////       std::vector<SQLTable*> implicitTables (resolveImplicitFrom(session_, from));
 ////       fromTables.insert( fromTables.begin(), implicitTables.begin(), implicitTables.end() );
 ////   }

    //table : IDENT '.' IDENT { SQLSession& s  = SQLSession::current(); $$ = s.findTable($1,$3); }
    for (size_t i(0); i < from.size(); ++i)
    {
        SQLTable& t (from[i]);
        fromTables.push_back(session_.findTable(t.name()));
    }

	Expressions select;
    for (Expressions::size_type i (0); i < select_list.size(); ++i)
	{
        L << "expandStars: " << *select_list[i] << std::endl;
		select_list[i]->expandStars(fromTables, select);
	}

////	ASSERT(maxColumnShift_ >= 0);
////	ASSERT(minColumnShift_ <= 0);
////	if (minColumnShift_ < 0)
////    {
////        L << std::endl << "SELECT_LIST before reshifting:" << select << std::endl;
////		reshift(select);
////        L << "SELECT_LIST after reshifting:" << select << std::endl << std::endl;
////
////        if (where)
////        {
////            L << std::endl << "WHERE before reshifting:" << *where << std::endl;
////            where = reshift(where);
////            L << "WHERE after reshifting:" << *where << std::endl << std::endl;
////        }
////
////        reshift(order_by.first);
////    }

	maxColumnShift_ = 0;
	minColumnShift_ = 0;

//	if (group_by.size())
//        Log::info() << "GROUP BY clause seen and ignored. Non aggregated values on select list will be used instead." << std::endl;

//    SQLOutput *out (createOutput(session_, into, order_by.first.size()));
    SQLOutput& out (createOutput(into, 0));

    // TODO: Special outputs
    if(distinct)              { NOTIMP; } // out = new SQLDistinctOutput(out); }
   //// if(order_by.first.size()) { NOTIMP; } // out = new SQLOrderOutput(out, order_by); }
    ///r = new SQLSelect(select, fromTables, where, out, config_, all);
    r = new SQLSelect(select, fromTables, 0, out, all);

	maxColumnShift_ = 0;
	return r;
}

//MetaData SQLSelectFactory::toODAColumns(SQLSession& session, const TableDef& tableDef)
//{
//    std::ostream& L(eckit::Log::debug());
//
//    ColumnDefs columnDefs (tableDef.columns());
//    MetaData md(0);
//    for (size_t i(0); i < columnDefs.size(); ++i)
//    {
//        ColumnDef& c (columnDefs[i]);
//        L << "   " << c.name() << ":" << c.type() << std::endl;
//
//        SchemaAnalyzer& a (session.currentDatabase().schemaAnalyzer());
//        if (a.isBitfield(c.name())) {
//            const BitfieldDef& bf ( a.getBitfieldTypeDefinition(c.name()) );
//            md.addBitfield(c.name(), bf );
//        }
//        else {
//            ColumnType type (Column::type(c.type()));
//            if (type == BITFIELD)
//                md.addBitfield(c.name(), c.bitfieldDef());
//            else
//                md.addColumn(c.name(), c.type());
//        }
//
//        ASSERT( &md[i]->coder() );
//    }
//    return md;
//}

SQLOutput& SQLSelectFactory::createOutput (const std::string& into, size_t orderBySize)
{
    //// TODO: FIXME
    ////size_t maxOpenFiles ( ! context ? 100 : atoi(context->environment().lookup("maxOpenFiles", "100", *context).c_str()));
    //size_t maxOpenFiles ( 100);
    ////TODO: pass parameter into to defaultFormat
    //SQLOutput *r (NULL);
    ////if (config_.outputFormat() == "callback") return session.defaultOutput();

    //std::string outputFile ((config_.outputFormat() == "odb") ? config_.outputFile() : into);
    //Log::debug() << "SQLSelectFactory::createOutput: outputFile: '" << outputFile << "'" << std::endl;
    //if (! outputFile.size())
        return session_.output();

    NOTIMP;
//    TemplateParameters templateParameters;
//    TemplateParameters::parse(outputFile, templateParameters);
//    if (templateParameters.size())
//    {
//        NOTIMP;
////        r = new SQLODAOutput<DispatchingWriter>(new DispatchingWriter(outputFile, orderBySize ? 1 : maxOpenFiles));
//    }
//    else
//    {
//        SchemaAnalyzer& a (session.currentDatabase().schemaAnalyzer());
//        if (! a.tableKnown(outputFile)) {
//            NOTIMP;
////            r = new SQLODAOutput<Writer<> >(new Writer<>(outputFile));
//        } else
//        {
//            NOTIMP;
//            Log::info() << "Table in the INTO clause known (" << outputFile << ")" << std::endl;
////            const TableDef* tableDef (&a.findTable(outputFile));
////            r = new SQLODAOutput<Writer<> >(new Writer<>(outputFile), toODAColumns(session, *tableDef));
//        }
//    }
//    return r;
}

//----------------------------------------------------------------------------------------------------------------------

} // namespace sql
} // namespace eckit