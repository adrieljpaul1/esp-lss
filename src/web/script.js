  /*// Connect to the MQTT broker
  const client = mqtt.connect('wss://test.mosquitto.org:8081'); // Use your broker's WebSocket URL

  const topic = 'your/topic/here'; // Replace with your MQTT topic
  const display = document.getElementById('json-display');

  client.on('connect', () => {
    display.textContent = 'Connected to MQTT broker. Waiting for data...';
    display.classList.remove('error');
    client.subscribe(topic, (err) => {
      if (err) {
        display.textContent = `Failed to subscribe to topic: ${err.message}`;
        display.classList.add('error');
      }
    });
  });

  client.on('message', (receivedTopic, message) => {
    if (receivedTopic === topic) {
      try {
        const jsonData = JSON.parse(message.toString());
        display.textContent = JSON.stringify(jsonData, null, 2); // Pretty print JSON
      } catch (e) {
        display.textContent = `Error parsing JSON: ${e.message}`;
        display.classList.add('error');
      }
    }
  });

  client.on('error', (err) => {
    display.textContent = `Connection error: ${err.message}`;
    display.classList.add('error');
  });
  */