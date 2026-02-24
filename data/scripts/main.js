// import * as utils from "./utils.js"
// TODO: add data validation
window.addEventListener("load", function () {
    fetch("index/get", { method: "PUT" })
        .then(response => {
            return response.json();
        })
        .then(data => {
            if (data["state"] === "ON") {
                document.getElementById("state").innerText = "Включено";
                document.getElementById("state-div").style.backgroundColor = "#c5e384";
            } else if (data["state"] === "OFF") {
                document.getElementById("state").innerText = "Вимкнено";
                document.getElementById("state-div").style.backgroundColor = "wheat";
            }

            document.getElementById("use-schedule").checked = data["use_schedule"];
            if (!data["use_schedule"]) {
                document.getElementById("if-use-schedule").style.display = "none";
            } else {
                document.getElementById("if-use-schedule").style.display = "block";
            }
            document.getElementById("action1").value = data["action1"];
            document.getElementById("action2").value = data["action2"];
            document.getElementById("date1").value = data["date1"];
            document.getElementById("date2").value = data["date2"];
            document.getElementById("time1").value = data["time1"];
            document.getElementById("time2").value = data["time2"];
            document.getElementById("repeat").value = data["repeat"];
            if (data["repeat"] !== "days") {
                document.getElementById("if-days").style.display = "none";
            } else {
                document.getElementById("if-days").style.display = "block";
            }
            document.getElementById("monday").checked = data["monday"];
            document.getElementById("tuesday").checked = data["tuesday"];
            document.getElementById("wednesday").checked = data["wednesday"];
            document.getElementById("thursday").checked = data["thursday"];
            document.getElementById("friday").checked = data["friday"];
            document.getElementById("saturday").checked = data["saturday"];
            document.getElementById("sunday").checked = data["sunday"];
        })
        .catch(error => {
            console.log("Error: ", error);
        })
});

document.getElementById("use-schedule").addEventListener("change", function () {
    let checked = this.checked;
    const schedule = document.getElementById("if-use-schedule");
    if (!checked) {
        schedule.style.display = "none";
    } else {
        schedule.style.display = "block";
    }
});

document.getElementById("repeat").addEventListener("change", function () {
    let selected_option = this.value;
    const days = document.getElementById("if-days");
    if (selected_option !== "days") {
        days.style.display = "none";
    } else {
        days.style.display = "block";
    }
});

document.getElementById("on").addEventListener("click", function () {
    fetch("/on", { method: "PUT" })
        .then(response => {
            if (response.status === 200) {
                document.getElementById("state").innerText = "Включено";
                document.getElementById("state-div").style.backgroundColor = "#c5e384";
            }
            return response.json();
        })
        .then(data => {
            console.log(data);
        })
        .catch(error => {
            console.log("Error: ", error);
        });
});

document.getElementById("off").addEventListener("click", function () {
    fetch("/off", { method: "PUT" })
        .then(response => {
            if (response.status === 200) {
                document.getElementById("state").innerText = "Вимкнено";
                document.getElementById("state-div").style.backgroundColor = "wheat";
            }
            return response.json();
        })
        .then(data => {
            console.log(data);
        })
        .catch(error => {
            console.log("Error: ", error);
        });
});

document.getElementById("submit").addEventListener("click", function () {
    let data = {};

    const use_schedule = document.getElementById("use-schedule").checked;
    const action1 = document.getElementById("action1").value;
    const action2 = document.getElementById("action2").value;
    const date1 = document.getElementById("date1").value;
    const date2 = document.getElementById("date2").value;
    const time1 = document.getElementById("time1").value;
    const time2 = document.getElementById("time2").value;
    const repeat = document.getElementById("repeat").value;
    const monday = document.getElementById("monday").checked;
    const tuesday = document.getElementById("tuesday").checked;
    const wednesday = document.getElementById("wednesday").checked;
    const thursday = document.getElementById("thursday").checked;
    const friday = document.getElementById("friday").checked;
    const saturday = document.getElementById("saturday").checked;
    const sunday = document.getElementById("sunday").checked;

    data["use_schedule"] = use_schedule;
    data["action1"] = action1;
    data["action2"] = action2;
    data["date1"] = date1;
    data["date2"] = date2;
    data["time1"] = time1;
    data["time2"] = time2;
    data["repeat"] = repeat;
    data["monday"] = monday;
    data["tuesday"] = tuesday;
    data["wednesday"] = wednesday;
    data["thursday"] = thursday;
    data["friday"] = friday;
    data["saturday"] = saturday;
    data["sunday"] = sunday;

    fetch("/schedule/set", {
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

    window.location.replace("/");
});

if (!!window.EventSource) {
    let events = new EventSource("/events");

    // events.onerror = function(event) {
    //     alert("З'єднання втрачено, перевірте ваш пристрій!");
    //     console.log(event);
    // };

    events.addEventListener("ping", function (event) {
        const data = JSON.parse(event.data);
        if (data["state"] === "ON") {
            document.getElementById("state").innerText = "Включено";
            document.getElementById("state-div").style.backgroundColor = "#c5e384";
        } else if (data["state"] === "OFF") {
            document.getElementById("state").innerText = "Вимкнено";
            document.getElementById("state-div").style.backgroundColor = "#wheat";
        }
    });

    events.addEventListener("ping_status", function (event) {
        const data = JSON.parse(event.data);
        if (data["status"] === "online") {
            document.getElementById("status").innerText = "Онлайн";
            document.getElementById("status").style.color = "green";
        } else if (data["status"] === "offline") {
            document.getElementById("status").innerText = "Оффлайн";
            document.getElementById("status").style.color = "red";
        }
    });

    events.addEventListener("update_action1", function (event) {
        document.getElementById("action1").value = event.data;
    });

    events.addEventListener("update_action2", function (event) {
        document.getElementById("action2").value = event.data;
    });

    events.addEventListener("update_date1", function (event) {
        document.getElementById("date1").value = event.data;
    });

    events.addEventListener("update_date2", function (event) {
        document.getElementById("date2").value = event.data;
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