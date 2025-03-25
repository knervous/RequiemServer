const fs = require('fs');

let structFile = fs.readFileSync('../../../common/patches/web_structs.h').toString();
const split = structFile.split('\n')

let inStruct = false;
let currStruct = {
    name: '',
    fields: []
};

const structs = [];

const splitCaps = string => string
    .replace(/([a-z])([A-Z]+)/g, (m, s1, s2) => s1 + ' ' + s2)
    .replace(/([A-Z])([A-Z]+)([^a-zA-Z0-9]*)$/, (m, s1, s2, s3) => s1 + s2.toLowerCase() + s3)
    .replace(/([A-Z]+)([A-Z][a-z])/g, 
        (m, s1, s2) => s1.toLowerCase() + ' ' + s2);
const snakeCase = string =>
    splitCaps(string)
        .replace(/\W+/g, " ")
        .split(/ |\B(?=[A-Z])/)
        .map(word => word.toLowerCase())
        .join('_');

for (const [idx, line] of Object.entries(split)) {
    const trimmedLine = line.trim();
    if (trimmedLine === '') {
        continue;
    }
    if (trimmedLine.startsWith('/')) {
        continue;
    }

    if (!inStruct && trimmedLine.startsWith('struct')) {
        inStruct = true;
        currStruct.name = trimmedLine.split(' ')[1];
        continue;
    }

    if (inStruct) {
        if (trimmedLine === '};') {
            inStruct = false;
            structs.push(currStruct);
            currStruct = {
                name: '',
                fields: []
            }
            continue;
        }

        const [content] = trimmedLine.split(';');
        const field = {};
        if (content.startsWith('struct')) {
            let [,structType,name] = content.split(' ');
            const repeated = name.includes('*') || name.includes('[')
            

            name = name.replace(/[\[\]\d+\*]/g, '');
            split[idx] = line.replace(name, snakeCase(name))

            field.name = name;
            field.type = structType;
            field.repeated = repeated;
            currStruct.fields.push(field)
            continue;
        }

        let [type, name] = content.split(' ');
        if (!name) {
            continue
        }
        if (type.includes('char')) {
            field.type = 'string';
            name = name.replace(/[\[\]\d+\*]/g, '');
            field.name = name;
            split[idx] = line.replace(name, snakeCase(name))
        } else {
            const repeated = name.includes('*') || name.includes('[')
            name = name.replace(/[\[\]\d+\*]/g, '');
            split[idx] = line.replace(name, snakeCase(name))
            field.name = name;
            field.type = type;
            field.repeated = repeated;
        }
        if (JSON.stringify(field) !== '{}') {
            currStruct.fields.push(field)
        }
    }
}

let protomessage = ``;

const getType = type => {
    if (type === 'string') return 'string'
    if (type.includes('int')) return 'int32'
    return type.replace('_Struct', '').replace('enum', '')
}
for (const struct of structs) {
    protomessage += `message ${struct.name.replace('_Struct', '')} {
        `
    for (const [idx, field] of Object.entries(struct.fields)) {
        if (field.name === 'next' && field.repeated) {
            continue;
        }
        protomessage += `${field.repeated? 'repeated ' : '' }${getType(field.type)} ${field.name} = ${+idx + 1};
        `
    }
    protomessage += `
    }
    
    `
}

fs.writeFileSync('gen.proto', protomessage)