// Firebase Config
var firebaseConfig = {
  apiKey: "<-copy apiKey here->",
  authDomain: "<-copy authDomain here->",
  projectId: "<-copy projectID here->",
  storageBucket: "<-copy storageBucket here->",
  messagingSenderId: "<-copy messagingSederID here->",
  appId: "<-copy appID here->"
};

// Initialize firebase
firebase.initializeApp(firebaseConfig);
// Get reference to auth
const auth = firebase.auth();

// Email/Password Login
document.getElementById("loginForm").addEventListener("submit", function(e) {
  e.preventDefault();
  // Get email
  const email = document.getElementById("email").value;
  // Get password
  const password = document.getElementById("password").value;

  // Sign in to firebase
  auth.signInWithEmailAndPassword(email, password)
    .then((userCredential) => {
      const user = userCredential.user;
      // Save email
      sessionStorage.setItem("userEmail", user.email);
      // Direct to search.html
      window.location.href = "search.html";
    })
    // If failed throw error
    .catch((error) => {
      document.getElementById("message").innerText = "Invalid login credentials.";
    });
});

// Google Login
function loginWithGoogle() {
  // Create google provider
  const provider = new firebase.auth.GoogleAuthProvider();

  // Sign in with google popup
  auth.signInWithPopup(provider)
    .then((result) => {
      const user = result.user;
      // Save email
      sessionStorage.setItem("userEmail", user.email);
      document.getElementById("message2").innerText = `Logged in as: ${user.displayName}`;
      // Redirect to seach.html
      window.location.href = "search.html";
    })
    // If failed throw error
    .catch((error) => {
      document.getElementById("message2").innerText = "Google login failed";
    });
}
