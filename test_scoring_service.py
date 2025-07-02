#!/usr/bin/env python3
"""
Test Scoring Service for MT4/MT5 A/B-Book Router
This is a simple test service that accepts scoring requests and returns mock scores.
In production, this would be replaced with your actual ML scoring service.
"""

import socket
import json
import struct
import threading
import time
import random
from datetime import datetime

class TestScoringService:
    def __init__(self, host='127.0.0.1', port=8080):
        self.host = host
        self.port = port
        self.running = False
        self.socket = None
        
    def start(self):
        """Start the scoring service"""
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        try:
            self.socket.bind((self.host, self.port))
            self.socket.listen(5)
            self.running = True
            
            print(f"Test Scoring Service started on {self.host}:{self.port}")
            print("Ready to receive scoring requests...")
            
            while self.running:
                try:
                    client_socket, client_address = self.socket.accept()
                    print(f"Connection from {client_address}")
                    
                    # Handle client in a separate thread
                    client_thread = threading.Thread(
                        target=self.handle_client,
                        args=(client_socket, client_address)
                    )
                    client_thread.daemon = True
                    client_thread.start()
                    
                except socket.error as e:
                    if self.running:
                        print(f"Socket error: {e}")
                        
        except Exception as e:
            print(f"Failed to start service: {e}")
        finally:
            if self.socket:
                self.socket.close()
    
    def handle_client(self, client_socket, client_address):
        """Handle individual client connection"""
        try:
            # Read message length (4 bytes, little-endian)
            length_data = client_socket.recv(4)
            if len(length_data) != 4:
                print(f"Invalid length data from {client_address}")
                return
                
            message_length = struct.unpack('<I', length_data)[0]
            print(f"Expecting message of {message_length} bytes")
            
            if message_length > 1024000:  # 1MB limit
                print(f"Message too large: {message_length} bytes")
                return
            
            # Read the actual message
            message_data = b''
            while len(message_data) < message_length:
                chunk = client_socket.recv(message_length - len(message_data))
                if not chunk:
                    break
                message_data += chunk
            
            if len(message_data) != message_length:
                print(f"Incomplete message received: {len(message_data)}/{message_length}")
                return
            
            # Parse JSON request (simplified protobuf simulation)
            try:
                request_json = message_data.decode('utf-8')
                request = json.loads(request_json)
                print(f"Received request for symbol: {request.get('symbol', 'UNKNOWN')}")
                
                # Generate response
                response = self.generate_score(request)
                response_json = json.dumps(response)
                response_bytes = response_json.encode('utf-8')
                
                # Send response length and data
                response_length = struct.pack('<I', len(response_bytes))
                client_socket.sendall(response_length)
                client_socket.sendall(response_bytes)
                
                print(f"Sent score: {response['score']:.6f}")
                
            except json.JSONDecodeError as e:
                print(f"JSON decode error: {e}")
            except Exception as e:
                print(f"Error processing request: {e}")
                
        except Exception as e:
            print(f"Error handling client {client_address}: {e}")
        finally:
            client_socket.close()
    
    def generate_score(self, request):
        """Generate a mock score based on the request"""
        # Simple scoring logic based on various factors
        score = 0.5  # Base score
        
        # Factor in symbol type
        symbol = request.get('symbol', '')
        inst_group = request.get('inst_group', 'Other')
        
        if inst_group == 'FXMajors':
            score += random.uniform(-0.2, 0.2)
        elif inst_group == 'Crypto':
            score += random.uniform(-0.3, 0.4)  # More volatile
        elif inst_group == 'Metals':
            score += random.uniform(-0.15, 0.15)
        else:
            score += random.uniform(-0.1, 0.1)
        
        # Factor in trade size
        lot_volume = float(request.get('lot_volume', 0.1))
        if lot_volume > 10.0:
            score += 0.1  # Large trades slightly more likely to be B-book
        elif lot_volume < 0.1:
            score -= 0.05  # Small trades slightly more likely to be A-book
        
        # Factor in account balance
        opening_balance = float(request.get('opening_balance', 1000))
        if opening_balance > 100000:
            score -= 0.1  # Large accounts more likely to be A-book
        elif opening_balance < 1000:
            score += 0.1  # Small accounts more likely to be B-book
        
        # Factor in risk management
        has_sl = int(request.get('has_sl', 0))
        has_tp = int(request.get('has_tp', 0))
        if has_sl and has_tp:
            score -= 0.05  # Good risk management slightly favors A-book
        
        # Factor in concurrent positions
        concurrent_positions = int(request.get('concurrent_positions', 0))
        if concurrent_positions > 10:
            score += 0.05  # Many concurrent positions slightly favors B-book
        
        # Add some randomness to simulate model uncertainty
        score += random.uniform(-0.05, 0.05)
        
        # Clamp score to reasonable range
        score = max(0.0, min(1.0, score))
        
        warnings = []
        if lot_volume > 50.0:
            warnings.append("Large position size detected")
        if opening_balance < 100:
            warnings.append("Low account balance")
        
        return {
            "score": score,
            "warnings": warnings,
            "timestamp": datetime.now().isoformat(),
            "request_id": f"req_{int(time.time())}"
        }
    
    def stop(self):
        """Stop the scoring service"""
        self.running = False
        if self.socket:
            self.socket.close()

def main():
    service = TestScoringService()
    
    try:
        service.start()
    except KeyboardInterrupt:
        print("\nShutting down service...")
        service.stop()

if __name__ == "__main__":
    main() 