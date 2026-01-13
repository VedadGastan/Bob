# Bob Programming Language

Bob je interpretirani programski jezik visokog nivoa dizajniran kao imperativni jezik opste namjene sa podrskom za paralelno programiranje.

## Karakteristike

- **Dinamicki tipiziran** - varijable nemaju eksplicitne tipove
- **Imperativna paradigma** - sa elementima funkcionalnog programiranja
- **Funkcije prvog reda** - funkcije kao vrijednosti
- **Ugradjena paralelizacija** - `parallel` petlje za visenitno izvrsavanje
- **Atomicne operacije** - thread-safe pristup dijeljenim podacima
- **C-ovska sintaksa** - citljiva i intuitivna

## Instalacija

### Preduslov
- C++ kompajler (g++) sa podrskom za C++17
- Windows operativni sistem

### Kompajliranje

```bash
build.bat build
```

## Pokretanje

### Pokretanje Bob programa

```bash
build.bat run <ime_fajla.bob>
```

Primjer:
```bash
build.bat run examples/hello.bob
```

## Tipovi podataka

- **Number** - decimalni brojevi (64-bit floating point)
- **String** - tekstualni nizovi
- **Bool** - `true` / `false`
- **Nil** - null vrijednost
- **Array** - dinamicki nizovi
- **Function** - funkcije kao objekti

## Osnovna sintaksa

### Varijable

```bob
var x = 10;
var ime = "Bob";
var brojevi = [1, 2, 3, 4, 5];
```

### Funkcije

```bob
func saberi(a, b) {
    return a + b;
}

var rezultat = saberi(5, 3);
print(rezultat);  // 8
```

### Kontrolne strukture

```bob
// If-elif-else
if (x > 10) {
    print("Vece od 10");
} elif (x > 5) {
    print("Vece od 5");
} else {
    print("5 ili manje");
}

// While petlja
var i = 0;
while (i < 5) {
    print(i);
    i++;
}

// For petlja
for (var i = 0; i < 10; i++) {
    print(i);
}
```

## Paralelno programiranje

### Parallel petlja

```bob
var podaci = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

parallel (var i = 0; i < len(podaci); i++) {
    podaci[i] = podaci[i] * podaci[i];
}

print(podaci);  // [1, 4, 9, 16, 25, 36, 49, 64, 81, 100]
```

### Atomicne operacije

```bob
var brojac = 0;

parallel (var i = 0; i < 1000; i++) {
    atomic_inc("brojac");
}

print(brojac);  // 1000
```

#### Dostupne atomicne funkcije:

- `atomic_store(var, val)` - atomicno postavljanje
- `atomic_load(var)` - atomicno citanje
- `atomic_add(var, val)` - atomicno dodavanje
- `atomic_sub(var, val)` - atomicno oduzimanje
- `atomic_inc(var)` - atomicni inkrement
- `atomic_dec(var)` - atomicni dekrement
- `atomic_xchg(var, new_val)` - atomicna zamjena
- `atomic_cas(var, expected, new_val)` - compare-and-swap

## Standardna biblioteka

### Osnovne funkcije

```bob
print("Hello World");           // Ispis
var duzina = len([1, 2, 3]);   // Duzina niza/stringa
var unos = input("Unesite: "); // citanje unosa
```

### Matematicke funkcije

```bob
sqrt(16)        // 4 - korijen
pow(2, 3)       // 8 - stepen
abs(-5)         // 5 - apsolutna vrijednost
floor(3.7)      // 3 - zaokruzivanje dole
ceil(3.2)       // 4 - zaokruzivanje gore
round(3.5)      // 4 - zaokruzivanje
sin(x)          // sinus
cos(x)          // kosinus
tan(x)          // tangens
log(x)          // prirodni logaritam
```

### Operacije sa nizovima

```bob
var lista = [1, 2, 3];
push(lista, 4);              // Dodaj na kraj
var zadnji = pop(lista);     // Ukloni zadnji element
```

### Pomocne funkcije

```bob
random()         // Slucajan broj [0, 1)
time()           // Vrijeme u milisekundama
sleep(1000)      // Pauza (ms)
thread_id()      // ID trenutne niti
num_threads()    // Broj dostupnih niti
```

## Operatori

### Aritmeticki
```bob
+ - * / %        // Osnovne operacije
**               // Stepenovanje
++ --            // Inkrement/dekrement
+= -= *= /= %=   // Slozena dodjela
```

### Relacioni
```bob
== != < <= > >=
```

### Logicki
```bob
and or not
```

### Operator clanstva
```bob
3 in [1, 2, 3]           // true
"World" in "Hello World" // true
```

## Primjeri

### Fibonacijev niz

```bob
func fibonacci(n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

for (var i = 0; i < 10; i++) {
    print(fibonacci(i));
}
```

### Paralelna obrada matrice

```bob
var matrica = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];

parallel (var i = 0; i < len(matrica); i++) {
    for (var j = 0; j < len(matrica[i]); j++) {
        matrica[i][j] = matrica[i][j] * 2;
    }
}

print(matrica);
```

### Funkcije viseg reda

```bob
func primijeni(niz, funkcija) {
    var rezultat = [];
    for (var i = 0; i < len(niz); i++) {
        push(rezultat, funkcija(niz[i]));
    }
    return rezultat;
}

func kvadrat(x) {
    return x * x;
}

var brojevi = [1, 2, 3, 4, 5];
var kvadrati = primijeni(brojevi, kvadrat);
print(kvadrati);  // [1, 4, 9, 16, 25]
```

## Struktura projekta

```
Bob/
??? main.cpp              # Glavni C++ fajl interpretera
??? build.bat             # Build skripta
??? README.md             # Ova dokumentacija
??? examples/             # Primjeri Bob programa
    ??? *.bob
```

## Autori

- Vedad Gastan (19685)
- Naila Delalic (19687)

**Univerzitet u Sarajevu**  
**Elektrotehnicki fakultet Sarajevo**  
**Odsjek za racunarstvo i informatiku**

---

*Programski jezici i prevodioci - Decembar 2025*