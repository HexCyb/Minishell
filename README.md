> Developed together with **Burak Yılmaz** ([@Ropnez](https://github.com/Ropnez)) at 42 Türkiye · original repository: [Ropnez/Minishell](https://github.com/Ropnez/Minishell)
>
> Bu proje 42 Türkiye'de Burak Yılmaz ile birlikte geliştirildi.

# Minishell Projesi

## İçindekiler
- [Hakkında](#hakkında)
- [Özellikler](#özellikler)
- [Kurulum](#kurulum)
- [Kullanım](#kullanım)
- [Teşekkür](#teşekkür)

## Hakkında
Minishell, 42 Ecole programının bir parçası olarak geliştirilmiş basit bir shell uygulamasıdır. Bu proje, temel shell komutlarının nasıl çalıştığını anlamanızı ve kendi shell uygulamanızı yazabilmenizi sağlamak amacıyla oluşturulmuştur.

## Özellikler
- Temel shell komutlarını çalıştırma
- Borulama (Piping)
- Yeniden yönlendirme (Redirection)
- Ortam değişkenlerini yönetme
- Basit hata ayıklama ve hata mesajları

## Kurulum
Minishell'i kendi makinenizde çalıştırmak için aşağıdaki adımları izleyin.

### Gereksinimler
- GCC veya benzeri bir C derleyicisi
- Make

### Adımlar
1. Depoyu klonlayın:
    ```sh
    git clone https://github.com/ropnez/minishell.git
    ```
2. Proje dizinine gidin:
    ```sh
    cd minishell
    ```
3. Projeyi derleyin:
    ```sh
    make
    ```

## Kullanım
Minishell'i çalıştırmak için terminalde aşağıdaki komutu kullanın:
```sh
./minishell
```

Minishell, temel shell komutlarını çalıştırmanızı sağlar. Örneğin:
```sh
ls -l
echo "Merhaba Dünya"
cat dosya.txt | grep aranan_kelime
```

## Teşekkür
Bu projede yardımlarından dolayı vparlak'a teşekkür ederim.
