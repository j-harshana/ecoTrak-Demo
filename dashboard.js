// Firebase Config
const firebaseConfig = {
    apiKey: "<-copy apiKey here->",
    authDomain: "<-copy authDomain here->",
    databaseURL: "<-copy databaseURL here->",
    projectId: "<-copy projectID here->",
    storageBucket: "<-copy storageBucket here->",
    messagingSenderId: "<-copy messagingSederID here->",
    appId: "<-copy appID here->"
  };

  // Initialize firebase
  firebase.initializeApp(firebaseConfig);

  // Get reference to realtime data and auth
  const database = firebase.database();
  const auth = firebase.auth();

  // Get the value of the readVehcile
  const licensePlate = sessionStorage.getItem("readVehicle");

  // MQ2 Chart
  const ctxMQ2 = document.getElementById("mq2Chart").getContext("2d");
  const chartMQ2 = new Chart(ctxMQ2, {
  type: "line",
  data: {
    labels: [], // Time
    datasets: [{
      label: "MQ2 (ppm)",
      data: [], // Sensor data
      borderColor: "green",
      backgroundColor: "rgba(0, 255, 0, 0.2)",
      fill: true,
      tension: 0.3,
      segment: {

        // Change color based on value levels
        borderColor: ctx => {
          const val = ctx.p0.parsed.y;
          if (val > 9000) return 'red';
          if (val >= 7000) return 'yellow';
          return 'green';
        },
        backgroundColor: ctx => {
          const val = ctx.p0.parsed.y;
          if (val > 9000) return 'rgba(255,0,0,0.2)';
          if (val >= 7000) return 'rgba(255,255,0,0.2)';
          return 'rgba(0,255,0,0.2)';
        }
      }
    }]
  },
  options: {
    responsive: true,
    maintainAspectRatio: true,
    aspectRatio: 2,
    scales: {
      x: {
        title: { display: true, text: "Time", color: "white" },
        ticks: { color: "white" }
      },
      y: {
        title: { display: true, text: "ppm", color: "white" },
        ticks: { color: "white" }
      }
    }
  }
});

  // MQ9 Chart
  const ctxMQ9 = document.getElementById("mq9Chart").getContext("2d");
  const chartMQ9 = new Chart(ctxMQ9, {
    type: "line",
    data: {
      labels: [], // Time
      datasets: [{
        label: "MQ9 (ppm)",
        data: [], // Sensor data
        borderColor: "green",
        backgroundColor: "rgba(0, 255, 0, 0.2)",
        fill: true,
        tension: 0.3
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: true,
      aspectRatio: 2,
      scales: {
        x: {
          title: { display: true, text: "Time", color: "white" },
          ticks: { color: "white" }
        },
        y: {
          title: { display: true, text: "ppm", color: "white" },
          ticks: { color: "white" }
        }
      }
    }
  });

  // Get latest data
  function fetchSensorData(sensor, chart, latestEl, maxEl) {
    const path = `vehicles/${licensePlate}/readings/${sensor}`;
    const ref = database.ref(path).orderByKey().limitToLast(50);

    ref.on("value", (snapshot) => {
      const data = snapshot.val();
      if (!data) return;

      let labels = [], values = [], latestVal = "--", maxVal = 0;

      // Process each data entry
      Object.keys(data).forEach(key => {
        const r = data[key];
        if (r && r.value !== undefined && r.time) {
          labels.push(r.time);
          values.push(r.value);
          latestVal = r.value;
          if (r.value > maxVal) maxVal = r.value;
        }
      });

      // Update chart data
      chart.data.labels = labels;
      chart.data.datasets[0].data = values;
      chart.update();

      // Update max and latest data
      const latestElem = document.getElementById(latestEl);
      const maxElem = document.getElementById(maxEl);

      latestElem.innerText = `${latestVal} ppm`;
      maxElem.innerText = `${maxVal} ppm`;

      // Set color based on value levels
      if (sensor === "MQ2") {
        if (maxVal > 1000) {
          maxElem.style.color = 'red';
        } else if (maxVal >= 800) {
          maxElem.style.color = 'yellow';
        } else {
          maxElem.style.color = 'white';
        }
      }

    });
  }

  // Logout and go back to index.html
  function logout() {
    sessionStorage.removeItem("readVehicle");
    database.ref("readVehicle").remove();
    firebase.auth().signOut().then(() => window.location.href = "index.html");
  }

  // Show data if user is logged in
  firebase.auth().onAuthStateChanged(user => {
    if (user && licensePlate) {
      fetchSensorData("MQ2", chartMQ2, "latestReadingMQ2Val", "maxReadingMQ2Val");
      fetchSensorData("MQ9", chartMQ9, "latestReadingMQ9Val", "maxReadingMQ9Val");
    } else {
      // Redirect to index if not authenticated
      window.location.href = "index.html";
    }
  });
