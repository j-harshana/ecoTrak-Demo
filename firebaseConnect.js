// Firebase Config
var firebaseConfig = {
    apiKey: "<-copy apiKey here->",
    authDomain: "<-copy authDomain here->",
    databaseURL: "<-copy databaseURL here->",
    projectId: "<-copy projectID here->",
    storageBucket: "<-copy storageBucket here->",
    messagingSenderId: "<-copy messagingSederID here->",
    appId: "<-copy appID here->"
    };

    // Initialize Firebase
    firebase.initializeApp(firebaseConfig);

    // Firebase services
    const auth = firebase.auth();
    const database = firebase.database();

    // Wait for auth state before allowing writes
    firebase.auth().onAuthStateChanged(function(user) {
      if (user) {
        // If the user is authenticated
        console.log("Authenticated as:", user.email);

        // Get the value of the licencePlate
        document.getElementById("loginBtn").addEventListener("click", () => {
          const licensePlateValue = document.getElementById("licensePlate").value.trim();

          // If license plate value is empty
          if (licensePlateValue === "") {
            document.getElementById("message").textContent = "Please enter a license plate.";
            return;
          }

          // get the license plate value and assign it to readVehicle
          database.ref("readVehicle").set(licensePlateValue)
            .then(() => {
              console.log("License plate saved.");
              sessionStorage.setItem("readVehicle", licensePlateValue);
              window.location.href = "dashboard.html";
            })
            // If fails throw error
            .catch((error) => {
              console.error("Database write error:", error);
              document.getElementById("message").textContent = "Write failed: " + error.message;
            });
        });

      } else {
        // Redirect if not logged in
        window.location.href = "index.html";
      }
    });
