function bypassClick() {
    let bypass = document.getElementById("bypass").checked;
    setParamNormalized(100, bypass ? 1 : 0);
    console.log("Bypass set: " + bypass);
}

function panChange() {
    let value = document.getElementById("pan").value;
    value = value / 100.0;
    setParamNormalized(102, value);
    console.log("Pan set: " + value);
}