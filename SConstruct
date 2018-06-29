
# Configuration for the tools
projectTools = 'tools/build/{}'

tools = {
	'bootstrap': ['tools/build/bootstrap.cpp'],
	'peer': ['tools/build/peer.cpp']
}

# Variant build directory specifications
VariantDir('tools/build', 'tools') # for tools
VariantDir('build', 'src') # for source code

# Command lines specifications
vars = Variables(None, ARGUMENTS)
# Set the list of the available tools
vars.Add(ListVariable('tools', 'List of tools to install', [], tools.keys()))

AddOption('--no-lib', action='store_false', dest='buildLib')

env = Environment(variables = vars,
				  CXXFLAGS = '-std=c++11',
				  LIBS = ['opendht', 'gnutls'])

# Build tools
for tool in env['tools']:
	env.Program(projectTools.format(tool), tools[tool])
# Build the library
if GetOption('buildLib') != False:
	env.SharedLibrary('build/ODScene', ['build/OdsNode.cpp'])
