import hashlib
import sys

def gerar_hash_senha(senha):
    return hashlib.sha256(senha.encode()).hexdigest()

if __name__ == "__main__":
    senha = sys.argv[1] if len(sys.argv) > 1 else "admin"
    print(gerar_hash_senha(senha))
