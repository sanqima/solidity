/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-3.0
/**
 * Compiler that transforms Yul Objects to EVM bytecode objects.
 */

#include <libyul/backends/evm/EVMObjectCompiler.h>

#include <libyul/backends/evm/EVMCodeTransform.h>
#include <libyul/backends/evm/EVMDialect.h>

#include <libyul/Object.h>
#include <libyul/Exceptions.h>

using namespace solidity::yul;
using namespace std;

void EVMObjectCompiler::compile(Object& _object, AbstractAssembly& _assembly, EVMDialect const& _dialect, bool _evm15, bool _optimize)
{
	EVMObjectCompiler compiler(_assembly, _dialect, _evm15);
	compiler.run(_object, _optimize);
}

void EVMObjectCompiler::run(Object& _object, bool _optimize)
{
	BuiltinContext context;
	context.currentObject = &_object;

	for (auto& subNode: _object.subObjects)
		if (Object* subObject = dynamic_cast<Object*>(subNode.get()))
		{
			auto subAssemblyAndID = m_assembly.createSubAssembly();
			context.subIDs[subObject->name] = subAssemblyAndID.second;
			compile(*subObject, *subAssemblyAndID.first, m_dialect, m_evm15, _optimize);
		}
		else
		{
			Data const& data = dynamic_cast<Data const&>(*subNode);
			context.subIDs[data.name] = m_assembly.appendData(data.data);
		}

	yulAssert(_object.analysisInfo, "No analysis info.");
	yulAssert(_object.code, "No code.");
	// We do not catch and re-throw the stack too deep exception here because it is a YulException,
	// which should be native to this part of the code.
	CodeTransform transform{m_assembly, *_object.analysisInfo, *_object.code, m_dialect, context, _optimize, m_evm15};
	transform(*_object.code);
	if (!transform.stackErrors().empty())
		BOOST_THROW_EXCEPTION(transform.stackErrors().front());
}
