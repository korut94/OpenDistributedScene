
# Configuration for the tools
projectTools = 'tools/build/{}'

tools = {
	'bootstrap': ['tools/build/bootstrap.cpp'],
	'peer': ['tools/build/peer.cpp']
}

# Variant build directory specifications
VariantDir('tools/build', 'tools')

# Command lines specifications
vars = Variables(None, ARGUMENTS)
# Set the list of the available tools
vars.Add(ListVariable('tools', 'List of tools to install', 0, tools.keys()))

env = Environment(variables = vars,
				  CXXFLAGS = '-std=c++11',
				  LIBS = ['opendht', 'gnutls'])

# Tools build
for tool in env['tools']:
	env.Program(projectTools.format(tool), tools[tool])
