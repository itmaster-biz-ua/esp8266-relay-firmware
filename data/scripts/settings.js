window.addEventListener("load", function () {
    fetch("settings/get", { method: "PUT" })
        .then(response => {
            return response.json();
        })
        .then(data => {
            document.getElementById("use-auth").checked = data["use_auth"];
            if (data["use_auth"]) {
                document.getElementById("if-use-auth").style.display = "block";
            }
            document.getElementById("use-static-ip").checked = data["use_static_ip"];
            if (data["use_static_ip"]) {
                document.getElementById("if-use-static-ip").style.display = "block";
            }
            document.getElementById("auth-pass").value = data["auth_pass"];
            document.getElementById("auth-login").value = data["auth_login"];
            document.getElementById("time-zone").value = data["time_zone"];
            document.getElementById("wifi-ssid").value = data["wifi_ssid"];
            document.getElementById("wifi-password").value = data["wifi_password"];
            document.getElementById("static-ipv4").value = data["static_ipv4"];
            document.getElementById("static-gateway").value = data["static_gateway"];
            document.getElementById("static-subnet").value = data["static_subnet"];
            document.getElementById("static-dns").value = data["static_dns"];

        })
        .catch(error => {
            console.log("Error: ", error);
        })


});

document.getElementById("submit").addEventListener("click", function () {
    let data = {};

    const use_auth = document.getElementById("use-auth").checked;
    const auth_pass = document.getElementById("auth-pass").value;
    if (auth_pass.length > 15)
    {
        alert("Пароль авторизації надто довгий.\nМаксимальна довжина пароля 15 символів.");
        return;
    }
    const auth_login = document.getElementById("auth-login").value;
    if (auth_login.length > 15)
    {
        alert("Пароль авторизації надто довгий.\nМаксимальна довжина пароля 15 символів.");
        return;
    }
    const time_zone = document.getElementById("time-zone").value;
    const wifi_ssid = document.getElementById("wifi-ssid").value;
    if (wifi_ssid.length > 32)
    {
        alert("WiFi SSID надто довгий.\nМаксимальна довжина SSID 32 символи.");
        return;
    }
    const wifi_password = document.getElementById("wifi-password").value;
    if (wifi_password.length > 32)
    {
        alert("WiFi пароль надто довгий.\nМаксимальна довжина пароля 32 символи.");
        return;
    }
    const use_static_ip = document.getElementById("use-static-ip").checked;
    const static_ipv4 = document.getElementById("static-ipv4").value;
    if (!validateIP(static_ipv4))
    {
        alert("Невірний формат IPv4-адреси");
        return;
    }
    const static_gateway = document.getElementById("static-gateway").value;
    if (!validateIP(static_gateway))
    {
        alert("Неправильний формат шлюзу");
        return;
    }
    const static_subnet = document.getElementById("static-subnet").value;
    if (!validateIP(static_subnet))
    {
        alert("Невірний формат маски підмережі");
        return;
    }
    const static_dns = document.getElementById("static-dns").value;
    if (!validateIP(static_dns))
    {
        alert("Неправильний формат DNS-суфікса");
        return;
    }

    data["use_auth"] = use_auth;
    data["auth_pass"] = auth_pass;
    data["auth_login"] = auth_login;
    data["time_zone"] = time_zone;
    data["wifi_ssid"] = wifi_ssid;
    data["wifi_password"] = wifi_password;
    data["use_static_ip"] = use_static_ip;
    data["static_ipv4"] = static_ipv4;
    data["static_gateway"] = static_gateway;
    data["static_subnet"] = static_subnet;
    data["static_dns"] = static_dns;

    fetch("/settings/set", {
        method: "POST",
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(data),
    })
        .then(response => response.json())
        .then(data => console.log(data))
        .catch((error) => {
            console.error('Error:', error);
        });

    window.location.replace("/settings");
});

document.getElementById("use-static-ip").addEventListener("change", function () {
    let checked = this.checked;
    const schedule = document.getElementById("if-use-static-ip");
    if (!checked) {
        schedule.style.display = "none";
    } else {
        schedule.style.display = "block";
    }
});

document.getElementById("use-auth").addEventListener("change", function () {
    let checked = this.checked;
    const schedule = document.getElementById("if-use-auth");
    if (!checked) {
        schedule.style.display = "none";
    } else {
        schedule.style.display = "block";
    }
});

if (!!window.EventSource) {
    let events = new EventSource("/events");

    events.addEventListener("ping_status", function (event) {
        const data = JSON.parse(event.data);
        if (data["status"] === "online") {
            document.getElementById("status").innerText = "Онлайн";
            document.getElementById("status").style.color = "green";
            document.getElementById("if-offline").style.display = "none";
        } else if (data["status"] === "offline") {
            document.getElementById("status").innerText = "Оффлайн";
            document.getElementById("status").style.color = "red";
            document.getElementById("if-offline").style.display = "flex";
        }
    });

    events.addEventListener("sync_time", function (_event) {
        let date = new Date();
        let hh = date.getHours();
        let mm = date.getMinutes();
        let ss = date.getSeconds();

        let YY = date.getFullYear().toString();
        let MM = date.getMonth() + 1;
        let DD = date.getDate();

        MM = (MM < 10) ? "0" + MM : MM;
        DD = (DD < 10) ? "0" + DD : DD;

        let formattedDate = YY + "-" + MM + "-" + DD;

        hh = (hh < 10) ? "0" + hh : hh;
        mm = (mm < 10) ? "0" + mm : mm;
        ss = (ss < 10) ? "0" + ss : ss;

        let time = hh + ":" + mm + ":" + ss;

        if (date === "" || time === "")
            alert("Поля дата та час мають бути заповнені.");
        else {
            let data = {};
            data["date_time"] = formattedDate + " " + time;

            fetch("/settings/time/set", {
                method: "POST",
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(data),
            })
                .then(response => {
                    if (response.status == 500)
                        alert("Wrond data format");
                    return response.json();
                })
                .then(data => {
                    console.log(data);
                })
                .catch(error => {
                    console.log("Error: ", error);
                })
        }
    });
}

function validateIP(ip) {
    let is_valid = true;
    let count = 0;

    ip.split(".").forEach(function(element) {
        count++;
        if (element < 0 || element > 255 || isNaN(element) || element === "") {
            is_valid = false;
        }
    });

    if (count != 4) {
        is_valid = false;
    }

    return is_valid;
}