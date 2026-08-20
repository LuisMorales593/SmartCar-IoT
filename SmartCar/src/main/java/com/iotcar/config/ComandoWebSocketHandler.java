package com.iotcar.config;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.web.socket.CloseStatus;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketSession;
import org.springframework.web.socket.handler.TextWebSocketHandler;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class ComandoWebSocketHandler extends TextWebSocketHandler {

	// Mapa para guardar las sesiones WebSocket de cada vehículo (vehiculoId ->
	// sesión)
	private static final Map<Long, WebSocketSession> sesiones = new ConcurrentHashMap<>();
	private static final ObjectMapper objectMapper = new ObjectMapper();

	@Override
	public void afterConnectionEstablished(WebSocketSession session) throws Exception {
		System.out.println("🔌 Nuevo ESP32 conectado: " + session.getId());
		// Se espera que el ESP32 envíe un mensaje de identificación
	}

	@Override
	protected void handleTextMessage(WebSocketSession session, TextMessage message) throws Exception {
		String payload = message.getPayload();
		System.out.println("📩 Mensaje recibido: " + payload);

		try {
			JsonNode json = objectMapper.readTree(payload);
			String tipo = json.get("tipo").asText();

			if ("identificar".equals(tipo)) {
				Long vehiculoId = json.get("vehiculoId").asLong();
				// Guardar la sesión asociada al vehículo
				sesiones.put(vehiculoId, session);
				System.out.println("✅ ESP32 identificado con vehiculoId: " + vehiculoId);
				session.sendMessage(new TextMessage("{\"tipo\":\"identificado\",\"vehiculoId\":" + vehiculoId + "}"));
			}
		} catch (Exception e) {
			System.err.println("❌ Error al procesar mensaje: " + e.getMessage());
		}
	}

	@Override
	public void afterConnectionClosed(WebSocketSession session, CloseStatus status) throws Exception {
		// Eliminar la sesión del mapa cuando se desconecte
		sesiones.entrySet().removeIf(entry -> entry.getValue().equals(session));
		System.out.println("🔌 ESP32 desconectado: " + session.getId());
	}

	// Método para enviar un comando a un vehículo específico
	public static void enviarComando(Long vehiculoId, String comando) {
		WebSocketSession session = sesiones.get(vehiculoId);
		if (session != null && session.isOpen()) {
			try {
				session.sendMessage(new TextMessage(comando));
				System.out.println("📤 Comando enviado a vehículo " + vehiculoId + ": " + comando);
			} catch (Exception e) {
				System.err.println("❌ Error al enviar comando: " + e.getMessage());
			}
		} else {
			System.out.println("⚠️ Vehículo " + vehiculoId + " no conectado por WebSocket.");
		}
	}

	// Método para verificar si un vehículo está conectado por WebSocket
	public static boolean estaConectado(Long vehiculoId) {
		WebSocketSession session = sesiones.get(vehiculoId);
		return session != null && session.isOpen();
	}
}