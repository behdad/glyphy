var fs = new ActiveXObject("Scripting.FileSystemObject");

var enumerator = new Enumerator(fs.GetFolder(WScript.Arguments.Item(0)).Files);
for (; !enumerator.atEnd(); enumerator.moveNext()) {
    var filePath = enumerator.item() + '';
    // skip if it is not a glsl file
    if (!/\.glsl$/.exec(filePath)) { continue; }
    var file = fs.OpenTextFile(filePath);
    var text = file.ReadAll().split('\n');
    file.Close();

    var result = fs.CreateTextFile(filePath.replace(/\.glsl$/, '-glsl.h'), true);
    result.WriteLine('static const char* ' + filePath.replace(/.*\\/, '').replace(/[-\.]/g, '_') + ' = ');
    for (var i in text) {
        var line = (text[i] + '');
        result.WriteLine('"' + line.replace(/\\/g, '\\\\').replace(/"/g, '\\"') + '\\n"');
    }
    result.WriteLine(';');
    result.Close();
}