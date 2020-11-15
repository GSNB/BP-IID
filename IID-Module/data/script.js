var devicesListJSON = new Object;
var postData;

const transFormRequest = (formValues) => {
    const formData = new FormData();
    Object.keys(formValues).map((key) => {
        formData.append(key, formValues[key])
    })
    return formData;
}

async function getConfiguration() {
    const response = await fetch('/api/getconfiguration');
    if (!response.ok) {
        const message = `An error has occured: ${response.status}`;
        throw new Error(message);
    }
    const data = await response.json();
    console.log(data);
    return data;
}

async function saveBTSettings() {
    if (document.getElementById("deviceSelect").selectedOptions[0].innerHTML != "No devices found") {
        if ('savedDevice' in devicesListJSON && document.getElementById("deviceSelect").value == devicesListJSON.deviceSelect.Name) {
            postData = devicesListJSON.deviceSelect.Address;
        }
        else if ("devices" in devicesListJSON) {
            for (var i = 0; i < devicesListJSON.devices.length; i++) {
                if (devicesListJSON.devices[i].Name == document.getElementById("deviceSelect").value) {
                    postData = devicesListJSON.devices[i].Address;
                    break;
                }
            }
        }
        const response = axios.post("/api/savebtsettings", transFormRequest({
            Address: postData
        }));
        console.log(response);
        return response;
    } else {
        document.getElementById("deviceerr").style.maxHeight = "500px";
    }
}

async function saveWIFISettings() {
    var ssid = document.getElementsByName("ssid")[0].value;
    var password = document.getElementsByName("password")[0].value;

    if (ssid.length > 4 && password.length >= 8) {
        const response = axios.post("/api/savewifisettings", transFormRequest({
            SSID: ssid,
            Password: password
        }));
        console.log(response);
        return response;
    }
    else {
        if (ssid.length <= 4) {
            document.getElementById("ssiderr").style.maxHeight = "500px";
        }
        if (password.length <= 8) {
            document.getElementById("passerr").style.maxHeight = "500px";
        }
    }

}

window.onload = function(){
    getConfiguration().then(data => {
        if (data.devices.length > 0) {
            for (var i = 0; i < data.devices.length; i++) {
                var option = document.createElement("option");
                option.text = data.devices[i].Name;
                document.getElementById("deviceSelect").add(option);
            }
            devicesListJSON = data;
        } else {
            document.getElementById("deviceSelect").options[0].innerHTML = "No devices found";
        }
        document.getElementsByName("ssid")[0].value = data.wifi.SSID;
        if ('savedDevice' in data) {
            document.getElementById("deviceSelect").options[0].innerHTML = data.savedDevice.Name;
        }
    }).catch(error => {
        document.getElementById("deviceSelect").options[0].innerHTML = "No devices found";
        console.log(error.message); // 'An error has occurred: 404'
    });
};