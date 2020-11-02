#!/usr/bin/env python3

"""
Automatically updates parameterlist.txt by searching all *.hh files
for usage of getParam or getParamFromGroup.
"""

import os

# find the content of the given string between the first matching pair of opening/closing keys
def getEnclosedContent(string, openKey, closeKey):

    # cut off everything before the first occurence of openKey
    string = openKey + string.partition(openKey)[2]

    # get content between mathing pair
    rest = string.partition(closeKey)
    result, rest = rest[0] + closeKey, rest[2]
    while result.count(openKey) != result.count(closeKey):
        rest = rest.partition(closeKey)
        if rest[1] == '': raise IOError('Could not get content between "{}" and "{}" in given string "{}"'.format(openKey, closeKey, string))
        result, rest = result + rest[0] + closeKey, rest[2]

    return result.partition(openKey)[2].rpartition(closeKey)[0]

# extract a parameter from a given line
def extractParamName(line):

    # split occurrences of getParam<T>(CALLARGS) or getParamFromGroup<T>(CALLARGS)
    # into the template arguments T and the function arguments CALLARGS
    if 'getParamFromGroup<' in line:
        line = line.split('getParamFromGroup')[1]
        hasGroup = True
    elif 'getParam<' in line:
        line = line.split('getParam')[1]
        hasGroup = False
    else:
        return {}

    # TODO: Support this also
    if line.count('getParam') > 1:
        raise IOError('Cannot process multiple occurrences of "getParam" in one line')

    # remove trailing spaces and cut off everything behind semicolon
    line = line.strip('\n').strip(' ').split(';')[0]

    # extract template arg between '<' and '>'
    paramType = getEnclosedContent(line, '<', '>')

    # extract function arguments
    functionArgs = line.partition('<' + paramType + '>')[2]
    functionArgs = getEnclosedContent(functionArgs, '(', ')')

    if hasGroup: functionArgs = functionArgs.partition(',')[2]
    functionArgs = functionArgs.partition(',')
    paramName = functionArgs[0]
    defaultValue = None if not functionArgs[2] else functionArgs[2]

    paramType = paramType.strip(' ')
    paramName = paramName.strip(' ')
    if (defaultValue): defaultValue = defaultValue.strip(' ')

    # if interior spaces occur in the parameter name, we can't identify it
    if paramName[0] != '"' or paramName[-1] != '"' or ' ' in paramName:
        raise IOError("Could not correctly process parameter name")

    return {'paramType': paramType, 'paramName': paramName.strip('"'), 'defaultValue': defaultValue}

# extract all parameters from a given file
def getParamsFromFile(file):
    parameters = []
    errors = {}
    with open(file) as f:
        for lineIdx, line in enumerate(f):
            try:
                param = extractParamName(line);
                if param: parameters.append(param);
            except IOError as e:
                errors[lineIdx] = {'line': line.strip(), 'message': e}

    # print encountered errors
    if errors:
        print('\n\n{} paramter{} in file {} could not be retrieved automatically. Please check them yourself:'.format(len(errors), 's' if len(errors) > 1 else '', file))
        for lineIdx in errors:
            print("\n\t-> line {}: {}".format(lineIdx, errors[lineIdx]['line']))
            print("\t\t-> error message: {}".format(errors[lineIdx]['message']))

    return parameters

# search all *.hh files for parameters
# TODO: allow runtime args with extensions and folder(s) to be checked
parameters = []
rootDir = os.path.dirname(os.path.abspath(__file__)) + "/../../dumux"
for root, _, files in os.walk(rootDir):
    for file in files:
        if os.path.splitext(file)[1] == ".hh" and os.path.splitext(file)[0] != 'parameters':
            parameters.extend(getParamsFromFile(os.path.join(root, file)))

# make sorted dictionary of the entries
# treat duplicates (could have differing default values or type names - e.g. via aliases)
parameterDict = {}
for params in parameters:
    key = params['paramName']
    if key in parameterDict:
        parameterDict[key]['defaultValue'].append(params['defaultValue'])
        parameterDict[key]['paramType'].append(params['paramType'])
    else:
        parameterDict[key] = params
        parameterDict[key]['defaultValue'] = [params['defaultValue']]
        parameterDict[key]['paramType'] = [params['paramType']]
sortedParameterDict = {key: value for key, value in sorted(parameterDict.items())}

tableEntriesWithGroup = []
tableEntriesWithoutGroup = []
previousGroupEntry = None

for key in sortedParameterDict:

    entry = sortedParameterDict[key]
    hasGroup = True if entry['paramName'].count('.') != 0 else False
    groupEntry = '-' if not hasGroup else entry['paramName'].split('.')[0]
    paramName = entry['paramName'] if not hasGroup else entry['paramName'].partition('.')[2]

    # TODO: selection scheme in case of multiple occurrences? For now we use the first one
    paramType = entry['paramType'][0]
    defaultValue = entry['defaultValue'][0] if entry['defaultValue'][0] != None else ''

    if groupEntry != previousGroupEntry:
        previousGroupEntry = groupEntry
        if hasGroup: groupEntry = '\\b ' + groupEntry

    tableEntry = ' * | {} | {} | {} | {} | TODO: explanation |'.format(groupEntry, paramName , paramType , defaultValue)

    if hasGroup: tableEntriesWithGroup.append(tableEntry)
    else: tableEntriesWithoutGroup.append(tableEntry)

# combine entries
tableEntries = tableEntriesWithoutGroup + tableEntriesWithGroup

# make a backup of the old parameterlist.txt file
copyfile(rootDir + '/../doc/doxygen/extradoc/parameterlist.txt', rootDir + '/../doc/doxygen/extradoc/parameterlist_old.txt')

header = """/*!
 *\\file
 *\ingroup Parameter
 *
 *\\brief List of currently useable run-time parameters
 *
 * The listed run-time parameters are available in general,
 * but we point out that a certain model might not be able
 * to use every parameter!
 *
 * | Group       | Parameter    | Type       | Default Value     | Explanation |
 * | :-          | :-           | :-         | :-                | :-          |
 * | -           | ParameterFile | std::string| executable.input  | name of the parameter file |
"""

# overwrite the old parameterlist.txt file
with open(rootDir + '/../doc/doxygen/extradoc/parameterlist.txt', "w") as outputfile:
    outputfile.write(header)
    for e in tableEntries:
        outputfile.write(e + '\n')
    outputfile.write(' */\n')
